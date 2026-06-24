#pragma once
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../core/system.h"
#include "helpers/lock_guard.h"

/**
 * Message Protocol (newline-delimited):
 * - IDENTIFY: TYPE:IDENTIFY|MAC:XX:XX:XX:XX:XX:XX|NAME:ESP32_Device\n
 * - LOG:      TYPE:LOG|MAC:XX:XX:XX:XX:XX:XX|LEVEL:INFO|MSG:Hello World\n
 * - DATA:     TYPE:DATA|MAC:XX:XX:XX:XX:XX:XX|KEY:sensor|VALUE:25.5\n
 */

#define TCP_CLIENT_MESSAGE_MAX_SIZE 128

extern System sys;

class TCPClient : public IModule
{
public:
    const char *name() override { return "TCPClient"; }

    MODULE_COMMANDS();

    bool init() override
    {
        mutex = xSemaphoreCreateRecursiveMutex();
        return mutex != NULL;
    }

    void setServer(const char *host, uint16_t port = 9000)
    {
        strncpy(this->host, host, sizeof(this->host) - 1);
        this->host[sizeof(this->host) - 1] = '\0';

        this->port = port;

        LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient configured with HOST: %s:%d\n",
             this->host, this->port);

        configured = true;
    }

    void update() override
    {
        if (!configured)
            return;

        if (networkAvailable() && !macAddressSet)
            setMACAddress();

        if (isConnected())
        {
            handleIncoming();
            if (millis() - lastPing > keepAlive)
            {
                LOG(sys, "TCP client keep-alive timeout. Disconnecting...", SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN);
                disconnect();
            }
        }

        reconnect();
    }

    void onEvent(const Event &e) override
    {
        if (e.type == EVENT_TCP_SEND)
        {
            const auto &data = e.data.tcpData;
            sendData(data.key, data.value);
        }
        else if (e.type == EVENT_LOG)
        {
            const auto &data = e.data.log;
            sendLog(data.level, data.message);
        }
    }

    uint32_t eventMask() override { return EVENT_BIT(EVENT_TCP_SEND) | EVENT_BIT(EVENT_LOG); }

    uint32_t updateInterval() override { return 1000; }

    void setDeviceName(const char *name)
    {
        strncpy(deviceName, name, sizeof(deviceName) - 1);
        deviceName[sizeof(deviceName) - 1] = '\0';
        isNameSet = true;
    }

    void setKeepAlive(uint16_t milliseconds) { keepAlive = milliseconds; }

    bool networkAvailable() { return WiFi.isConnected(); }

    bool isConnected()
    {
        LockGuard lock(mutex);

        return networkAvailable() && configured && client.connected();
    }

    void disconnect()
    {
        LockGuard lock(mutex);

        if (client.connected())
            client.stop();

        lastPing = 0;
    }

private:
    // =============== VARIABLES ===============
    // Connection state
    bool configured = false;
    uint32_t lastAttempt = 0;

    // Ping timings
    uint32_t lastPing = 0;
    uint16_t keepAlive = 10 * 1000;

    // Buffer size limit
    const size_t maxBufferSize = 256;

    // Server configuration
    char host[17]; // Max length for IPv4 address string
    uint16_t port;

    // WiFi client instance
    WiFiClient client;

    // MAC address cache
    char macAddress[18]; // Format: XX:XX:XX:XX:XX:XX\0
    bool macAddressSet = false;

    // Device name for IDENTIFY message
    char deviceName[33];
    bool isNameSet = false;

    // RX buffer for incoming messages
    char rxBuffer[256];
    size_t rxPos = 0;

    // Mutex to protect WiFiClient across cores/tasks
    SemaphoreHandle_t mutex = NULL;

    // =============== COMMANDS ===============
    static constexpr ModuleCommand moduleCommands[] = {
        {"tcp.setServer", "Set the TCP server address and port", [](void *context, const Command &cmd) -> CommandResult {
             TCPClient *tcp = static_cast<TCPClient *>(context);

             if (cmd.argumentCount < 1)
                 return {false, "Missing argument: server address"};

             const char *host = cmd.arg(0);
             uint16_t port = 9000; // Default port

             if (cmd.argumentCount >= 2)
                 port = atoi(cmd.arg(1));

             tcp->setServer(host, port);
             return {true, "TCP server configured"};
         }},
        {"tcp.setDeviceName", "Set the device name for IDENTIFY message", [](void *context, const Command &cmd) -> CommandResult {
             TCPClient *tcp = static_cast<TCPClient *>(context);

             if (cmd.argumentCount < 1)
                 return {false, "Missing argument: device name"};

             tcp->setDeviceName(cmd.arg(0));
             return {true, "Device name set"};
         }},
        {"tcp.setKeepAlive", "Set the keep-alive timeout in milliseconds", [](void *context, const Command &cmd) -> CommandResult {
             TCPClient *tcp = static_cast<TCPClient *>(context);

             if (cmd.argumentCount < 1)
                 return {false, "Missing argument: keep-alive timeout"};

             uint16_t timeout = atoi(cmd.arg(0));
             tcp->setKeepAlive(timeout);
             return {true, "Keep-alive timeout set"};
         }}};
    // =============== FUNCTIONS ===============
    void reconnect()
    {
        if (!networkAvailable())
            return;

        uint32_t now = millis();
        // Attempt to reconnect if disconnected (every 10 seconds)
        if (!isConnected() && now - lastAttempt > 10000)
        {
            lastAttempt = now;

            LOGF(sys, SRC_TCP, LOG_INFO, LOG_COLOR_CYAN, "Attempting to connect to %s:%d...", host, port);

            bool ok;
            {
                LockGuard lock(mutex);
                ok = client.connect(host, port);
            }

            if (ok)
            {
                sendIdentifyMessage();
                lastPing = millis();
                LOGF(sys, SRC_TCP, LOG_INFO, LOG_COLOR_CYAN, "TCPClient connected at %lu! MAC: %s", lastPing, macAddress);
            }
            else
            {
                LOG(sys, "TCPClient connection failed!", SRC_TCP, LOG_ERROR, LOG_COLOR_RED);
            }
        }
    }

    void handleIncoming()
    {
        while (true)
        {
            bool hasByte = false;
            char c = 0;

            {
                LockGuard lock(mutex);
                if (client.available())
                {
                    c = client.read();
                    hasByte = true;
                }
            }

            if (!hasByte)
                break;

            if (c == '\n')
            {
                rxBuffer[rxPos] = '\0';
                processMessage(rxBuffer);
                rxPos = 0;

                LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient received: %s", rxBuffer);
            }
            else if (rxPos < sizeof(rxBuffer) - 1)
            {
                rxBuffer[rxPos++] = c;
            }
        }
    }

    void processMessage(const char *msg)
    {
        if (strncmp(msg, "TYPE:PING", 9) == 0)
        {
            lastPing = millis();
            sendPong();
        }
        else if (strncmp(msg, "TYPE:COMMAND", 12) == 0)
        {
            // Not implemented yet!
        }
    }

    void sendPong()
    {
        char pongBuffer[64];
        uint32_t now = millis();

        snprintf(pongBuffer, sizeof(pongBuffer),
                 "TYPE:PONG|MAC:%s|TS:%u\n",
                 macAddress,
                 now);

        LockGuard lock(mutex);

        client.println(pongBuffer);
    }

    bool sendLog(LogLevel level, const char *message)
    {
        if (!isConnected() ||
            strlen(message) == 0 ||
            strlen(message) > TCP_CLIENT_MESSAGE_MAX_SIZE)
            return false;

        char buffer[maxBufferSize];
        snprintf(buffer, sizeof(buffer),
                 "TYPE:LOG|MAC:%s|LEVEL:%s|MSG:%s",
                 macAddress, toString(level), message);

        LockGuard lock(mutex);

        client.println(buffer);

        return true;
    }

    bool sendData(const char *key, const char *value)
    {
        if (!isConnected() ||
            strlen(key) == 0 ||
            strlen(value) == 0 ||
            strlen(key) > 64 ||
            strlen(value) > TCP_CLIENT_MESSAGE_MAX_SIZE)
            return false;

        char buffer[maxBufferSize];
        snprintf(buffer, sizeof(buffer),
                 "TYPE:DATA|MAC:%s|KEY:%s|VALUE:%s",
                 macAddress, key, value);

        LockGuard lock(mutex);

        client.println(buffer);

        return true;
    }

    void sendIdentifyMessage()
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "TYPE:IDENTIFY|MAC:%s|NAME:%s",
                 macAddress, isNameSet ? deviceName : macAddress);

        LockGuard lock(mutex);

        client.println(buffer);
    }

    void setMACAddress()
    {
        WiFi.macAddress().toCharArray(macAddress, sizeof(macAddress));
        LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient MAC address set: %s", macAddress);
        macAddressSet = true;
    }
};
