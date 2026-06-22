#pragma once
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * Message Protocol (newline-delimited):
 * - IDENTIFY: TYPE:IDENTIFY|MAC:XX:XX:XX:XX:XX:XX|NAME:ESP32_Device\n
 * - LOG:      TYPE:LOG|MAC:XX:XX:XX:XX:XX:XX|LEVEL:INFO|MSG:Hello World\n
 * - DATA:     TYPE:DATA|MAC:XX:XX:XX:XX:XX:XX|KEY:sensor|VALUE:25.5\n
 */

#define TCP_CLIENT_MESSAGE_MAX_SIZE 128

class TCPClient : public IModule
{
public:
    const char *name() override { return "TCPClient"; }

    void setServer(const char *host, uint16_t port = 9000)
    {
        strncpy(this->host, host, sizeof(this->host) - 1);
        this->host[sizeof(this->host) - 1] = '\0';

        this->port = port;

        WiFi.macAddress().toCharArray(macAddress, sizeof(macAddress));

        Serial.printf("TCPClient configured with HOST: %s:%d | MAC: %s\n",
                      this->host, this->port, macAddress);

        isBegun = true;
    }

    void update() override
    {
        if (!isBegun)
            return;

        uint32_t now = millis();

        if (client.connected() && now - lastPing > keepAlive)
        {
            disconnect();
            Serial.println("TCP client timed out!");
        }

        reconnect();
    }

    uint32_t eventMask() override { return EVENT_BIT(EVENT_TCP_SEND); }

    uint32_t updateInterval() override { return 1000; }

    void setDeviceName(const char *name)
    {
        strncpy(deviceName, name, sizeof(deviceName) - 1);
        deviceName[sizeof(deviceName) - 1] = '\0';
        isNameSet = true;
    }

    bool isConnected()
    {
        return isBegun && client.connected();
    }

    void disconnect()
    {
        if (client.connected())
            client.stop();
    }

private:
    // =============== VARIABLES ===============
    // Connection state
    bool isBegun = false;
    uint32_t lastAttempt = 0;

    // Ping timings
    uint32_t lastPing = 0;
    const uint16_t keepAlive = 10 * 1000;

    // Buffer size limit
    const size_t maxBufferSize = 256;

    // Server configuration
    char host[17]; // Max length for IPv4 address string
    uint16_t port;

    // WiFi client instance
    WiFiClient client;

    // MAC address cache
    char macAddress[18]; // Format: XX:XX:XX:XX:XX:XX\0

    // Device name for IDENTIFY message
    char deviceName[33];
    bool isNameSet = false;

    // =============== FUNCTIONS ===============
    void reconnect()
    {
        uint32_t now = millis();
        // Attempt to reconnect if disconnected (every 10 seconds)
        if (!client.connected() && now - lastAttempt > 10000)
        {
            lastAttempt = now;

            Serial.printf("Attempting to connect to %s:%d...\n", host, port);

            if (client.connect(host, port))
            {
                Serial.printf("TCPClient connected! MAC: %s\n", macAddress);
                sendIdentifyMessage();
            }
            else
            {
                Serial.println("TCPClient connection failed!");
            }
        }
    }

    void handleIncoming()
    {
        char rxBuffer[256];
        size_t rxPos = 0;

        while (client.available())
        {
            char c = client.read();

            if (c == '\n')
            {
                rxBuffer[rxPos] = '\0';
                processMessage(rxBuffer);
                rxPos = 0;
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
            sendPong();

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

        client.println(pongBuffer);

        lastPing = now;
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

        client.println(buffer);
        return true;
    }

    void sendIdentifyMessage()
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "TYPE:IDENTIFY|MAC:%s|NAME:%s",
                 macAddress, isNameSet ? deviceName : macAddress);

        client.println(buffer);
    }
};
