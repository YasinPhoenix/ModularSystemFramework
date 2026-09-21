#pragma once
#include "../core/System.h"
#include "common/LockGuard.h"
#include "module_configs/TCPClientConfig.h"
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

struct MessageField {
    const char *key;
    const char *value;
};

class TCPClient : public IModule {
public:
    // =============== Default Values ===============
    static constexpr bool DEFAULT_AUTO_CONNECT = true;
    static constexpr uint16_t DEFAULT_KEEP_ALIVE = 5 * 1000;
    static constexpr int32_t DEFAULT_CONNECTION_TIMEOUT = 3 * 1000;
    static constexpr size_t DEFAULT_MAX_BUFFE_SIZE = 256;


    // =============== Constructor ===============
    explicit TCPClient(const TCPModuleConfig &cfg = {}) : initialConfig(cfg) {}

    // =============== Initial Configurations ===============
    uint8_t applyInitialConfig() {
        uint8_t result = 0;
        if (!initialConfig.server_address)
            result |= (1 << 0);
        if (!initialConfig.server_port)
            result |= (1 << 1);
        if (!(result & (1 << 0)) && !(result & (1 << 1))) {
            setServer(initialConfig.server_address, *initialConfig.server_port);
        } else
            result |= (1 << 2);

        if (initialConfig.device_name && !setDeviceName(initialConfig.device_name))
            result |= (1 << 3);

        if (initialConfig.keep_alive && !setKeepAlive(*initialConfig.keep_alive))
            result |= (1 << 4);
        if (initialConfig.connection_timeout && !setTimeout(*initialConfig.connection_timeout))
            result |= (1 << 5);
        if (initialConfig.auto_connect && !setAutoConnect(*initialConfig.auto_connect))
            result |= (1 << 6);

        return result;
    }

    void outputFailedParts(uint8_t res, char *buffer, size_t bufferSize) {
        snprintf(buffer, bufferSize, "%s%s%s%s%s",
                 (res & (1 << 2)) ? "Server " : "",
                 (res & (1 << 3)) ? "Device name " : "",
                 (res & (1 << 4)) ? "Keep-alive " : "",
                 (res & (1 << 5)) ? "connection timeout " : "",
                 (res & (1 << 6)) ? "Auto-connect" : "");
    }
    // =============== Functions ===============
    const char *name() override { return "TCPClient"; }

    MODULE_COMMANDS();

    bool init(System *sys) override {
        if (!sys) {
            LOG_ERROR(sys, "System wasn't given at module initiation!", SRC_TCP);
            return false;
        }

        this->sys = sys;

        char result[128];
        configAvailable = scope.init(name(), sys->getFileSystem(), result);
        LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_MAGENTA, "Config scope initialization result: %s\n", result);

        uint8_t res = applyInitialConfig();
        if (res != 0) {
            char fails[128];
            outputFailedParts(res, fails, sizeof(fails));
            LOGF(sys, SRC_TCP, LOG_WARN, LOG_COLOR_YELLOW, "Failed to apply TCP configurations: %s", fails);
        }
        loadConfig();

        mutex = xSemaphoreCreateMutex();
        if (mutex == NULL) {
            LOG_ERROR(sys, "Failed to initiate: mutex is NULL!", SRC_TCP);
            return false;
        }

        return true;
    }

    bool setServer(const char *host, uint16_t port = 9000) {
        if (!host) {
            LOG_ERROR(sys, "Failed to set server: No host address was given!", SRC_TCP);
            return false;
        }

        if (strlen(host) > 16)
            LOG_WARN(sys, "Loaded value from config for host address is bigger than expected!", SRC_TCP);

        strncpy(this->host, host, sizeof(this->host) - 1);
        this->host[sizeof(this->host) - 1] = '\0';

        char result[128];
        if (!scope.set(CONFIG_SERVER_ADDRESS, host, result))
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED, "Failed to save address: %s", result);

        if (!updatePort(port, true))
            return false;

        LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient configured with HOST: %s:%d\n",
             this->host, this->port);

        configured = true;
        return true;
    }

    bool updatePort(uint16_t port, bool fromSetServer = false) {
        if (!configured && !fromSetServer)
            LOG_WARN(sys, "The host address needs to be set!", SRC_TCP);

        if (port == 0) {
            LOG_ERROR(sys, "Failed to update port: Invalid port number!", SRC_TCP);
            return false;
        }

        this->port = port;

        char result[128];
        char intBuffer[6];

        snprintf(intBuffer, sizeof(intBuffer), "%u", port);

        if (!scope.set(CONFIG_SERVER_PORT, intBuffer, result))
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED, "Failed to save port: %s", result);

        LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient port updated to %d", port);
        return true;
    }

    void update() override {
        if (!configured)
            return;

        if (networkAvailable() && !macAddressSet)
            setMACAddress();

        if (isConnected()) {
            handleIncoming();
            if (millis() - lastPing > keepAlive) {
                LOG(sys, "TCP client keep-alive timeout. Disconnecting...", SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN);
                disconnect();
            }
        } else if (autoConnect) {
            reconnect();
        }
    }

    void onEvent(const Event &e) override {
        if (e.type == EVENT_TCP_SEND) {
            const auto &data = e.data.tcpData;
            sendData(data.key, data.value);
        } else if (e.type == EVENT_LOG) {
            const auto &data = e.data.log;
            sendLog(data.level, data.message);
        }
    }

    uint32_t eventMask() override { return EVENT_BIT(EVENT_TCP_SEND) | EVENT_BIT(EVENT_LOG); }

    uint32_t updateInterval() override { return 1000; }

    bool setDeviceName(const char *name) {
        if (strlen(name) > sizeof(deviceName)) {
            LOG_ERROR(sys, "Loaded value from config for device name is bigger than expected!", SRC_TCP);
            return false;
        }

        strncpy(deviceName, name, sizeof(deviceName) - 1);
        deviceName[sizeof(deviceName) - 1] = '\0';
        isNameSet = true;

        char result[128];
        if (!scope.set(CONFIG_DEVICE_NAME, name, result))
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED, "Failed to save device name: %s", result);
            
        return true;
    }

    bool setKeepAlive(uint16_t ka) {
        if (ka < DEFAULT_KEEP_ALIVE) {
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED,
                 "Keep-alive number invalid! (must be %u or above)", DEFAULT_KEEP_ALIVE);
            return false;
        }

        keepAlive = ka;

        char kaStr[6];
        snprintf(kaStr, sizeof(kaStr), "%u", keepAlive);

        char result[128];
        if (!scope.set(CONFIG_KEEP_ALIVE, kaStr, result))
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED, "Failed to save keepAlive: %s", result);

        return true;
    }

    bool setTimeout(int32_t to) {
        if (to < DEFAULT_CONNECTION_TIMEOUT) {
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED,
                 "Connection timeout number invalid! (must be %dms or above)", DEFAULT_CONNECTION_TIMEOUT);
            return false;
        }

        timeout = to;

        char toStr[6];
        snprintf(toStr, sizeof(toStr), "%d", timeout);

        char result[128];
        if (!scope.set(CONFIG_TIMEOUT, toStr, result))
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED, "Failed to save connection timeout: %s", result);

        return true;
    }

    bool setAutoConnect(bool enable) {
        autoConnect = enable;

        char result[128];
        if (!scope.set(CONFIG_AUTO_CONNECT, enable ? "1" : "0", result))
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_RED, "Failed to save autoConnect: %s", result);

        return true;
    }

    void setMAC(const char *value) {
        if (!value || value[0] == '\0') {
            LOG_ERROR(sys, "Failed to set MAC address: Empty string was given!", SRC_TCP);
            return;
        }

        snprintf(macAddress, sizeof(macAddress), "%s", value);
        macAddressSet = true;
    }

    bool networkAvailable() { return WiFi.isConnected(); }

    bool isConnected() {
        LockGuard lock(mutex);

        return networkAvailable() && configured && client.connected();
    }

    void disconnect() {
        LockGuard lock(mutex);

        if (client.connected())
            client.stop();

        lastPing = 0;
    }

private:
    // =============== VARIABLES ===============
    TCPModuleConfig initialConfig;

    ConfigScope scope;
    bool configAvailable = false;

    // Connection state
    bool configured = false;
    uint32_t lastAttempt = 0;
    bool autoConnect = DEFAULT_AUTO_CONNECT;

    // Ping timings
    uint32_t lastPing = 0;
    uint16_t keepAlive = DEFAULT_KEEP_ALIVE;
    int32_t timeout = DEFAULT_CONNECTION_TIMEOUT;

    // Buffer size limit
    const size_t maxBufferSize = DEFAULT_MAX_BUFFE_SIZE;

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

    // =============== FS Config Names ===============
    const char *CONFIG_SERVER_ADDRESS = "address";
    const char *CONFIG_SERVER_PORT = "port";
    const char *CONFIG_DEVICE_NAME = "deviceName";
    const char *CONFIG_KEEP_ALIVE = "keepAlive";
    const char *CONFIG_TIMEOUT = "timeout";
    const char *CONFIG_AUTO_CONNECT = "autoConnect";
    const char *CONFIG_MAC = "MAC";

    // =============== COMMANDS ===============

    static CommandResult CmdSetServer(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: server address"};

        const char *host = cmd.arg(0);
        uint16_t port = 9000; // Default port

        if (cmd.argumentCount >= 2)
            port = atoi(cmd.arg(1));

        bool success = tcp->setServer(host, port);

        return {success, success ? "TCP server configured!" : "Failed to configure TCP server!"};
    }

    static CommandResult CmdSetDeviceName(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: device name"};

        if (!tcp->setDeviceName(cmd.arg(0)))
            return {false, "Failed to set device name"};
    }

    static CommandResult CmdSetKeepAlive(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: keep-alive timeout"};

        uint16_t timeout = atoi(cmd.arg(0));
        if (tcp->setKeepAlive(timeout)) {
            return {true, "Keep-alive timeout set!"};
        } else {
            return {false, "Failed to set Keep-alive!"};
        }
    }

    static CommandResult CmdSetTimeout(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: Connection timeout"};

        int32_t timeout = atoi(cmd.arg(0));
        if (tcp->setTimeout(timeout)) {
            return {true, "Connection timeout set!"};
        } else {
            return {false, "Failed to set Connection timeout!"};
        }
    }

    static CommandResult CmdSetAutoConnect(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: auto-connect"};

        uint8_t val = atoi(cmd.arg(0));
        if (val > 1)
            return {false, "Failed to set auto-connect: invalid value"};

        tcp->setAutoConnect(val);
        return {true, "Auto-connect updated!"};
    }

    static CommandResult CmdConnect(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        bool autoReconnectSet = false;
        if (cmd.argumentCount) {
            uint8_t val = atoi(cmd.arg(0));
            if (val > 1) {
                LOG_ERROR(tcp->sys, "Failed to set auto-reconnect: Invalid value!", SRC_TCP);
            } else {
                tcp->setAutoConnect(val);
                autoReconnectSet = true;
            }
        }

        if (cmd.argumentCount > 1)
            LOG_WARN(tcp->sys, "This command only takes one argument. the rest are ignored!", SRC_TCP);

        if (tcp->isConnected()) {
            return {true, "Already connected!"};
        }

        tcp->reconnect();
        if (autoReconnectSet) {
            return {true, "Auto-connect enabled. Attempting to connect..."};
        } else {
            return {true, "Attempting to connect..."};
        }
    }

    static CommandResult CmdDisconnect(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        tcp->setAutoConnect(false);

        if (!tcp->isConnected()) {
            return {true, "Already disconnected!"};
        }

        tcp->disconnect();
        return {true, "Disconnected from server!"};
    }

    static constexpr ModuleCommand moduleCommands[] = {
        {"setServer", "Set the TCP server address and port <IP Address> [port=9000]", CmdSetServer},
        {"setDeviceName", "Set the device name for IDENTIFY message <name>", CmdSetDeviceName},
        {"setKeepAlive", "Set the keep-alive timeout in milliseconds <keep-alive>", CmdSetKeepAlive},
        {"setConnectTimeout", "Set the connection timeout in milliseconds <timeout>", CmdSetTimeout},
        {"setAutoConnect", "Set the auto-connect value <0=OFF|1=ON>", CmdSetAutoConnect},
        {"connect", "Connect to the TCP server and enable auto-reconnect [auto-reconnect: 0=OFF|1=ON]", CmdConnect},
        {"disconnect", "Disconnect from the TCP server and disable auto-reconnect", CmdDisconnect}};

    // =============== CONFIG ===============

    static void applyHost(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        LOGF(tcp->sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded host address from config: %s", value);

        tcp->setServer(value);
    }

    static void applyPort(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        LOGF(tcp->sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded port from config: %s", value);

        tcp->updatePort(atoi(value));
    }

    static void applyDeviceName(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        LOGF(tcp->sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded device name from config: %s", value);

        if (!tcp->setDeviceName(value))
            LOG_ERROR(tcp->sys, "Failed to apply loaded device name!", SRC_TCP);
    }

    static void applyKeepAlive(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        LOGF(tcp->sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded keep-alive from config: %s", value);

        tcp->setKeepAlive(atoi(value));
    }

    static void applyTimeout(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        LOGF(tcp->sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded connection timeout from config: %s", value);

        tcp->setTimeout(atoi(value));
    }

    static void applyAutoConnect(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        LOGF(tcp->sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded auto-connect from config: %s", value);

        uint8_t val = atoi(value) != 0;

        if (val > 1) {
            LOG_ERROR(tcp->sys, "Failed to load auto-connect from config: invalid value", SRC_TCP);
            return;
        }

        tcp->setAutoConnect(val);
    }

    static void applyMAC(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        TCPClient *tcp = static_cast<TCPClient *>(ctx);

        LOGF(tcp->sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded MAC from config: %s", value);

        tcp->setMAC(value);
    }

    bool loadConfig() {
        if (!configAvailable)
            return false;

        const ConfigField fields[] = {
            {CONFIG_SERVER_ADDRESS, applyHost},
            {CONFIG_SERVER_PORT, applyPort},
            {CONFIG_DEVICE_NAME, applyDeviceName},
            {CONFIG_KEEP_ALIVE, applyKeepAlive},
            {CONFIG_TIMEOUT, applyTimeout},
            {CONFIG_AUTO_CONNECT, applyAutoConnect},
            {CONFIG_MAC, applyMAC}};

        uint8_t availableCount = 0;

        for (const auto &field : fields) {
            for (const char *item : scope.items) {
                if (item[0] == '\0') // skip empty entries
                    continue;

                if (strcmp(field.key, item) == 0) {
                    const char *value = scope.get(field.key);
                    field.apply(this, value);

                    availableCount++;
                    break;
                }
            }
        }

        if (!availableCount)
            LOG_WARN(sys, "No TCP configurations available!", SRC_TCP);

        return true;
    }

    // =============== FUNCTIONS ===============

    bool reconnect() {
        if (!networkAvailable()) {
            LOG_ERROR(sys, "Network unavailable. Cannot reconnect!", SRC_TCP);
            return false;
        }

        uint32_t now = millis();
        // Attempt to reconnect if disconnected (every 10 seconds)
        if (!isConnected() && now - lastAttempt > 10000) {
            lastAttempt = now;

            LOGF(sys, SRC_TCP, LOG_INFO, LOG_COLOR_CYAN, "Attempting to connect to %s:%d...", host, port);

            bool ok;
            {
                LockGuard lock(mutex);
                ok = client.connect(host, port, timeout);
            }

            if (ok) {
                sendIdentifyMessage();

                while (sendCommands()) {
                    vTaskDelay(10); // Small delay to avoid flooding
                }

                lastPing = millis();
                LOGF(sys, SRC_TCP, LOG_INFO, LOG_COLOR_CYAN, "TCPClient connected at %lu! MAC: %s", lastPing, macAddress);
                return true;
            } else {
                LOG_ERROR(sys, "TCPClient connection failed!", SRC_TCP);
                return false;
            }
        }
        LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient reconnect attempt skipped. Last attempt was %lu ms ago.", now - lastAttempt);
        return false;
    }

    void handleIncoming() {
        while (true) {
            bool hasByte = false;
            char c = 0;

            {
                LockGuard lock(mutex);
                if (client.available()) {
                    c = client.read();
                    hasByte = true;
                }
            }

            if (!hasByte)
                break;

            if (c == '\n') {
                rxBuffer[rxPos] = '\0';
                processMessage(rxBuffer);
                rxPos = 0;

                LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient received: %s", rxBuffer);
            } else if (rxPos < sizeof(rxBuffer) - 1) {
                rxBuffer[rxPos++] = c;
            }
        }
    }

    const char *findField(const MessageField *fields, int count, const char *key) {
        for (int i = 0; i < count; ++i) {
            if (strcmp(fields[i].key, key) == 0)
                return fields[i].value;
        }

        return nullptr;
    }

    void processMessage(const char *msg) {
        char buffer[256];
        strncpy(buffer, msg, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        MessageField fields[10];
        int count = 0;

        char *savePtr;
        char *token = strtok_r(buffer, "|", &savePtr);

        while (token && count < 10) {
            char *colon = strchr(token, ':');

            if (colon) {
                *colon = '\0';
                fields[count++] = {token, colon + 1};
            }

            token = strtok_r(nullptr, "|", &savePtr);
        }

        const char *type = findField(fields, count, "TYPE");

        if (!type) {
            LOG(sys, "Message missing TYPE field", SRC_TCP, LOG_ERROR, LOG_COLOR_RED);
            return;
        }

        if (strcmp(type, "PING") == 0) {
            lastPing = millis();
            sendPong();
        } else if (strcmp(type, "EXECUTE") == 0) {
            const char *mac = findField(fields, count, "MAC");

            if (mac && strcmp(mac, macAddress) == 0) {
                const char *cmd = findField(fields, count, "COMMAND");

                if (!cmd) {
                    LOG(sys, "EXECUTE message missing COMMAND field", SRC_TCP, LOG_ERROR, LOG_COLOR_RED);
                    return;
                }

                auto result = sys->executeCommand(cmd);
                if (result.success) {
                    LOGF(sys, SRC_SERIAL, LOG_DEBUG, LOG_COLOR_GREEN, "Command executed successfully: %s", result.message);
                } else {
                    LOGF(sys, SRC_SERIAL, LOG_ERROR, LOG_COLOR_RED, "Command execution failed: %s", result.message);
                }
            }
        } else {
            LOGF(sys, SRC_TCP, LOG_WARN, LOG_COLOR_YELLOW, "Unknown message type: %s", type);
        }
    }

    bool sendCommands() {
        static uint8_t index = 0;
        CommandInfo info;

        if (sys->getCommandInfo(index, info)) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                     "TYPE:COMMANDS|MAC:%s|MODULE_NAME:%s|COMMAND_NAME:%s|HELP:%s",
                     macAddress, info.moduleName, info.name, info.help ? info.help : "");

            LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "Sending command info: %s", buffer);

            LockGuard lock(mutex);

            client.println(buffer);
            index++;
            return true;
        }

        index = 0; // Reset for next time
        return false;
    }

    void sendPong() {
        char pongBuffer[64];
        uint32_t now = millis();

        snprintf(pongBuffer, sizeof(pongBuffer), "TYPE:PONG|MAC:%s|TS:%u\n", macAddress, now);

        LockGuard lock(mutex);

        client.println(pongBuffer);
    }

    bool sendLog(LogLevel level, const char *message) {
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

    bool sendData(const char *key, const char *value) {
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

    void sendIdentifyMessage() {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "TYPE:IDENTIFY|MAC:%s|NAME:%s",
                 macAddress, isNameSet ? deviceName : macAddress);

        LockGuard lock(mutex);

        client.println(buffer);
    }

    void setMACAddress() {
        if (macAddressSet)
            return;

        WiFi.macAddress().toCharArray(macAddress, sizeof(macAddress));
        LOGF(sys, SRC_TCP, LOG_DEBUG, LOG_COLOR_CYAN, "TCPClient MAC address set: %s", macAddress);
        macAddressSet = true;

        char result[128];
        if (!scope.set(CONFIG_MAC, macAddress, result))
            LOGF(sys, SRC_TCP, LOG_ERROR, LOG_COLOR_CYAN, "Failed to save MAC address: %s", result);
    }
};
