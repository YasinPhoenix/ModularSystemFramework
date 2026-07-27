#pragma once
#include <WiFi.h>

#include "../core/API.h"
#include "../core/System.h"
#include "../core/module/IModule.h"
#include "common/LogCommon.h"
#include "common/RetryManager.h"
#include "common/WiFiCommon.h"

class MyWiFiClient : public IModule {
public:
    const char *name() override { return "WiFi"; }

    MODULE_COMMANDS();

    bool init(System *sys) override {
        if (!sys) {
            LOG_ERROR(sys, "System wasn't given at module initiation!", SRC_WIFI);
            return false;
        }

        this->sys = sys;

        mutex = xSemaphoreCreateMutex();

        WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info) { this->onWiFiEvent(event, info); });
        WiFi.setAutoReconnect(false);

        wifiRetry.init(maxReconnectAttempts, reconnectIntervalMs, [this]() { return commence(); });

        wifiRetry.onExhaustedDo([this]() {
            LOG_ERROR(this->sys, "Max WiFi reconnection attempts reached. Reconnect aborted.", SRC_WIFI);
            autoReconnect = false;
        });

        wifiRetry.onAttemptFailedDo([this](uint8_t attempt) {
            LOG_WARN(this->sys, "WiFi reconnection attempt failed to start!", SRC_WIFI);
        });

        char result[128];
        configAvailable = scope.init(name(), sys->getFileSystem(), result);
        LOGF(sys, SRC_WIFI, LOG_DEBUG, LOG_COLOR_MAGENTA, "Config scope initialization result: %s\n", result);

        loadConfig();

        return true;
    }

    void update() override {
        bool staApplicable = (config.mode == WiFiMode::STA || config.mode == WiFiMode::AP_STA) && config.hasStaCred();

        WiFiConnectionState currentState;
        {
            LockGuard lock(mutex);
            currentState = state;
        } // released immediately after copying the value out

        if (currentState == WiFiConnectionState::DISCONNECTED && autoReconnect && staApplicable) {

            if (!wifiRetry.isArmed())
                wifiRetry.arm();
            wifiRetry.update();
        }
    }

    uint32_t updateInterval() override { return 10; }

    bool setCred(const char *value, bool isSsid, bool isAp) {
        auto _config = isSsid ? (isAp ? config.apSsid : config.staSsid)
                              : (isAp ? config.apPass : config.staPass);

        if (!value || value[0] == '\0') {
            if (isSsid) {
                LOGF(sys, SRC_WIFI, LOG_ERROR, LOG_COLOR_RED,
                     "Failed to set %s SSID: No value was given!", isAp ? "AP" : "STA");
                return false;
            }
            // null password = clear it (open network) — bypasses length validation on purpose
            _config[0] = '\0';
            const char *path = isAp ? "apPass" : "staPass";
            char result[128];
            if (!scope.set(path, _config, result))
                LOGF(sys, SRC_WIFI, LOG_WARN, LOG_COLOR_YELLOW, "Failed to save %s password to config: %s", isAp ? "AP" : "STA", result);
            return true;
        }

        if (!(isSsid ? isSsidValid(value) : isPassValid(value))) {
            LOGF(sys, SRC_WIFI, LOG_ERROR, LOG_COLOR_RED,
                 "Failed to set %s %s: Invalid value: %s",
                 isAp ? "AP" : "STA", isSsid ? "SSID" : "password", value);
            return false;
        }

        auto _appliedConfig = isSsid ? (isAp ? appliedConfig.apSsid : appliedConfig.staSsid)
                                     : (isAp ? appliedConfig.apPass : appliedConfig.staPass);

        if (strcmp(_appliedConfig, value) != 0) {
            size_t maxLen = isSsid ? WIFI_SSID_MAX_LEN : WIFI_PASS_MAX_LEN;
            strncpy(_config, value, maxLen);
            _config[maxLen] = '\0';
        } else {
            LOGF(sys, SRC_WIFI, LOG_INFO, LOG_COLOR_CYAN,
                 "%s %s didn't change!",
                 isAp ? "AP" : "STA", isSsid ? "SSID" : "password");
        }

        const char *path = isSsid ? (isAp ? "apSsid" : "staSsid") : (isAp ? "apPass" : "staPass");
        char result[128];
        if (!scope.set(path, _config, result))
            LOGF(sys, SRC_WIFI, LOG_WARN, LOG_COLOR_YELLOW,
                 "Failed to save %s %s to config: %s",
                 isAp ? "AP" : "STA", isSsid ? "SSID" : "password", result);

        return true;
    }

    bool setCreds(const char *ssid, const char *pass, bool isAp) {
        bool ssidOK = setCred(ssid, true, isAp);
        bool passOK = setCred(pass, false, isAp);
        return ssidOK && passOK;
    }

    bool setMode(WiFiMode mode) {
        bool modeChanged = mode != appliedConfig.mode;

        if (!isModeValid(mode)) {
            LOGF(sys, SRC_WIFI, LOG_ERROR, LOG_COLOR_RED, "WiFi mode invalid: %d\n", mode);
            return false;
        }

        if (mode == appliedConfig.mode) {
            LOG_INFO(sys, "WiFi mode didn't change!", SRC_WIFI, LOG_COLOR_CYAN);
            return true;
        }

        config.mode = mode;

        if (!scope.set("mode", config.getModeStr(true)))
            LOG_ERROR(sys, "Failed to save WiFi mode to config", SRC_WIFI);

        return true;
    }

    inline void setAutoReconnect(bool enable) { autoReconnect = enable; }

    bool commence() {
        // Nothing changed & still connected
        WiFiConnectionState currentState;
        {
            LockGuard lock(mutex);
            currentState = state;
        } // released immediately after copying the value out

        if (config == appliedConfig && currentState != WiFiConnectionState::DISCONNECTED) {
            LOG_INFO(sys, "No changes were made to WiFi!", SRC_WIFI, LOG_COLOR_CYAN);
            return true;
        }

        // Getting the new mode
        wifi_mode_t newMode = WIFI_OFF;

        switch (config.mode) {
        case WiFiMode::STA:
            newMode = WIFI_STA;
            break;

        case WiFiMode::AP:
            newMode = WIFI_AP;
            break;

        case WiFiMode::AP_STA:
            newMode = WIFI_AP_STA;
            break;
        }

        // Change it if needed
        bool modeChanged = WiFi.getMode() != newMode;
        if (modeChanged) {
            if (!WiFi.mode(newMode)) {
                LOG_ERROR(sys, "Failed to change WiFi mode", SRC_WIFI);
                return false;
            } else {
                LOG_DEBUG(sys, "WiFi mode changed!", SRC_WIFI);
            }
        }

        bool staDropped =
            (config.mode != WiFiMode::STA && config.mode != WiFiMode::AP_STA) &&
            (appliedConfig.mode == WiFiMode::STA ||
             appliedConfig.mode == WiFiMode::AP_STA);

        if (staDropped) {
            LockGuard lock(mutex);
            state = WiFiConnectionState::DISCONNECTED;
            currentState = state;
            staIp = IPAddress();
        }

        // Reconfigure STA if needed
        bool staChanged = strcmp(config.staSsid, appliedConfig.staSsid) != 0 ||
                          strcmp(config.staPass, appliedConfig.staPass) != 0;

        if ((config.mode == WiFiMode::STA || config.mode == WiFiMode::AP_STA) &&
            (staChanged || modeChanged || currentState == WiFiConnectionState::DISCONNECTED)) {
            if (!WiFi.disconnect(false, false)) {
                LOG_ERROR(sys, "Failed to disconnect STA", SRC_WIFI);
                return false;
            } else if (currentState != WiFiConnectionState::DISCONNECTED) {
                {
                    LockGuard lock(mutex);
                    staIp = IPAddress();
                }
                LOG_DEBUG(sys, "STA disconnected!", SRC_WIFI);
            }

            if (config.hasStaCred()) {
                WiFi.begin(config.staSsid, config.staPass);

                {
                    LockGuard lock(mutex);
                    state = WiFiConnectionState::CONNECTING;
                }
                LOG_DEBUG(sys, "STA connecting...", SRC_WIFI);
            }
        }

        // Reconfigure AP if needed

        bool apChanged = strcmp(config.apSsid, appliedConfig.apSsid) != 0 ||
                         strcmp(config.apPass, appliedConfig.apPass) != 0;

        if ((config.mode == WiFiMode::AP || config.mode == WiFiMode::AP_STA) &&
            (apChanged || modeChanged)) {
            if (!WiFi.softAPdisconnect(true)) {
                LOG_ERROR(sys, "Failed to disable AP", SRC_WIFI);
                return false;
            } else {
                apIp = IPAddress();
                LOG_DEBUG(sys, "AP disabled!", SRC_WIFI);
            }

            if (config.hasApCred())
                if (!WiFi.softAP(config.apSsid, config.apPass)) {
                    LOG_ERROR(sys, "Failed to enable AP", SRC_WIFI);
                    return false;
                } else {
                    apIp = WiFi.softAPIP();
                    LOG_DEBUG(sys, "Enabled AP!", SRC_WIFI);
                }
        }

        // Save
        appliedConfig = config;
        return true;
    }

    bool stop() {
        if (!WiFi.disconnect(false, false)) {
            LOG_ERROR(sys, "Failed to disconnect STA", SRC_WIFI);
            return false;
        } else {
            {
                LockGuard lock(mutex);
                staIp = IPAddress();
            }
            LOG_DEBUG(sys, "Disconnected STA!", SRC_WIFI);
        }

        if (!WiFi.softAPdisconnect(true)) {
            LOG_ERROR(sys, "Failed to disable AP", SRC_WIFI);
            return false;
        } else {
            apIp = IPAddress();
            LOG_DEBUG(sys, "Disconnected AP!", SRC_WIFI);
        }

        if (!WiFi.mode(WIFI_OFF)) {
            LOG_ERROR(sys, "Failed to turn off the WiFi", SRC_WIFI);
            return false;
        } else
            LOG_DEBUG(sys, "WiFi turned off!", SRC_WIFI);

        wifiRetry.disarm();

        LockGuard lock(mutex);
        state = WiFiConnectionState::OFF;
        return true;
    }

    void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info) {
        switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            LOG_DEBUG(sys, "Waiting for IP!", SRC_WIFI);
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            {
                LockGuard lock(mutex);
                state = WiFiConnectionState::CONNECTED;
                staIp = IPAddress(info.got_ip.ip_info.ip.addr);
            }
            wifiRetry.reportSuccess();
            EVENT_WIFI_CONNECTED(SRC_WIFI);
            LOG_INFO(sys, "WiFi Connected!", SRC_WIFI, LOG_COLOR_CYAN);
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
            uint8_t reason = info.wifi_sta_disconnected.reason;

            bool wasOff;
            {
                LockGuard lock(mutex);
                wasOff = (state == WiFiConnectionState::OFF);
                if (!wasOff) {
                    state = WiFiConnectionState::DISCONNECTED;
                    staIp = IPAddress();
                }
            }

            if (wasOff)
                break;

            EVENT_WIFI_DISCONNECTED(SRC_WIFI);
            LOGF(sys, SRC_WIFI, LOG_WARN, LOG_COLOR_YELLOW, "WiFi Disconnected! reason=%d", reason);
            break;
        }
        }
    }

    inline WiFiConnectionState getConnectionState() const {
        LockGuard lock(mutex);
        return state;
    }

    inline bool isConnected() const {
        LockGuard lock(mutex);
        return state == WiFiConnectionState::CONNECTED;
    }

    inline WiFiMode getMode() const { return appliedConfig.mode; }
    inline uint8_t getClientCount() { return clientCount; }
    inline IPAddress getStaIp() const {
        LockGuard lock(mutex);
        return staIp;
    }

    inline IPAddress getApIp() const { return apIp; }

    WiFiConfig getConfig() const {
        WiFiConfig newConfig = config;

        memset(newConfig.staPass, 0, sizeof(newConfig.staPass));
        memset(newConfig.apPass, 0, sizeof(newConfig.apPass));
        return newConfig;
    }

private:
    // =============== VARIABLES ===============
    ConfigScope scope;
    bool configAvailable = false;

    WiFiConnectionState state = WiFiConnectionState::OFF;

    IPAddress staIp;
    IPAddress apIp;

    SemaphoreHandle_t mutex;

    volatile bool autoReconnect = true;
    uint32_t reconnectIntervalMs = 10 * 1000; // 10 seconds
    uint8_t maxReconnectAttempts = 5;

    RetryManager wifiRetry;

    uint8_t clientCount = 0;
    bool hasClientCountChanged = false;

    WiFiConfig config;
    WiFiConfig appliedConfig;

    // =============== COMMANDS ===============
    static CommandResult CMDSetSta(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount < 2)
            return {false, "Missing arguments: SSID and password"};

        if (!wifi->setCreds(cmd.arg(0), cmd.arg(1), false))
            return {false, "Invalid credentials. Check length requirements."};

        return {true, "STA credentials set successfully"};
    }

    static CommandResult CMDSetAp(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount < 2)
            return {false, "Missing arguments: SSID and password"};

        if (!wifi->setCreds(cmd.arg(0), cmd.arg(1), true))
            return {false, "Invalid credentials. Check length requirements."};

        return {true, "AP credentials set successfully"};
    }

    static CommandResult CMDSetMode(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: mode"};

        int mode = atoi(cmd.arg(0));
        if (mode < 0 || mode > 2)
            return {false, "Invalid mode. Must be 0 (STA), 1 (AP), or 2 (AP+STA)"};

        if (!wifi->setMode(static_cast<WiFiMode>(mode)))
            return {false, "Failed to set mode and reconnect"};

        return {true, "WiFi mode set successfully"};
    }

    static CommandResult CMDSetAutoReconnect(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: autoReconnect"};

        uint8_t ar = atoi(cmd.arg(0));
        if (ar > 1)
            return {false, "Invalid value. Must be 0 (OFF), or 1 (ON)"};

        wifi->setAutoReconnect(ar);
        return {true, "Auto reconnect changed!"};
    }

    static CommandResult CMDCommence(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount)
            LOG_WARN(wifi->sys, "WiFi commence doesn't accept any arguments! Arguments ignored...", SRC_WIFI);

        bool success = wifi->commence();
        return {success, success ? "WiFi successfully started!" : "Failed to start WiFi"};
    }

    static CommandResult CMDStop(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount)
            LOG_WARN(wifi->sys, "WiFi stop doesn't accept any arguments! Arguments ignored...", SRC_WIFI);

        bool success = wifi->stop();
        return {success, success ? "WiFi successfully stopped!" : "Failed to stop WiFi"};
    }

    static CommandResult CMDGetClientCount(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);
        LOGF(wifi->sys, SRC_WIFI, LOG_INFO, LOG_COLOR_MAGENTA, "AP client count: %d\n", wifi->getClientCount());
        return {true, "Success"};
    }

    static CommandResult CMDGetConfig(void *ctx, const Command &cmd) {
        if (!ctx)
            return {false, "Context is null!"};

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        WiFiConfig config = wifi->getConfig();
        LOGF(wifi->sys, SRC_WIFI, LOG_INFO, LOG_COLOR_MAGENTA,
             "WiFi saved credentials: { Mode: %s | STA SSID: %s | AP SSID: %s }",
             config.getModeStr(),
             config.staSsid[0] == '\0' ? "[NOT SET]" : config.staSsid,
             config.apSsid[0] == '\0' ? "[NOT SET]" : config.apSsid);
        return {true, "Success"};
    }

    static constexpr ModuleCommand moduleCommands[] = {
        {"setSta", "Set WiFi STA credentials <SSID> [password=\"\"]", CMDSetSta},
        {"setAp", "Set WiFi AP credentials <SSID> [password=\"\"]", CMDSetAp},
        {"setMode", "Set WiFi mode <0=STA|1=AP|2=AP+STA>", CMDSetMode},
        {"setAutoReconnect", "Set WiFi auto reconnect <0=OFF|1=ON>", CMDSetAutoReconnect},
        {"commence", "Commence WiFi network", CMDCommence},
        {"stop", "Stop WiFi module", CMDStop},
        {"clientCount", "Get AP client count", CMDGetClientCount},
        {"getConfig", "Get saved configuration!", CMDGetConfig},
    };

    // =============== CONFIG ===============
    static void applyMode(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        WiFiConfig config;
        config.setMode(atoi(value));

        wifi->setMode(config.mode);
        LOGF(wifi->sys, SRC_WIFI, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded WiFi mode: %s", config.getModeStr());
    }

    static void applyStaSsid(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        wifi->setCred(value, true, false);
        LOGF(wifi->sys, SRC_WIFI, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded STA SSID: %s", value);
    }

    static void applyStaPass(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        wifi->setCred(value, false, false);
        LOGF(wifi->sys, SRC_WIFI, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded STA password: %s", value);
    }

    static void applyApSsid(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        wifi->setCred(value, true, true);
        LOGF(wifi->sys, SRC_WIFI, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded AP SSID: %s", value);
    }

    static void applyApPass(void *ctx, const char *value) {
        if (!ctx || !value)
            return;

        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        wifi->setCred(value, false, true);
        LOGF(wifi->sys, SRC_WIFI, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded AP password: %s", value);
    }

    bool loadConfig() {
        if (!configAvailable)
            return false;

        const ConfigField fields[] = {
            {"mode", applyMode},
            {"staSsid", applyStaSsid},
            {"staPass", applyStaPass},
            {"apSsid", applyApSsid},
            {"apPass", applyApPass}};

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
            LOG_WARN(sys, "No WiFi configurations available!", SRC_WIFI);

        return true;
    }

    // =============== FUNCTIONS ===============
    bool isSsidValid(const char *ssid) {
        if (!ssid)
            return false;

        size_t len = strlen(ssid);
        if (len == 0 || len > WIFI_SSID_MAX_LEN)
            return false;

        return true;
    }
    bool isPassValid(const char *pass) {
        if (!pass)
            return false;

        size_t len = strlen(pass);
        if (len > WIFI_PASS_MAX_LEN || len < WIFI_PASS_MIN_LEN)
            return false;

        return true;
    }

    inline bool areCredentialsValid(const char *ssid, const char *pass) { return isSsidValid(ssid) && isPassValid(pass); }

    bool isModeValid(WiFiMode mode) {
        switch (mode) {
        case WiFiMode::STA:
        case WiFiMode::AP:
        case WiFiMode::AP_STA:
            return true;
        default:
            return false;
        }
    }
};
