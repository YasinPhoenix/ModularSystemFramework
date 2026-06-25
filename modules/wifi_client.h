#pragma once
#include <WiFi.h>
#include "../core/imodule.h"
#include "../core/system.h"
#include "../core/api.h"
#include "helpers/wifi_helpers.h"

class MyWiFiClient : public IModule
{
public:
    const char *name() override { return "WiFi"; }

    MODULE_COMMANDS();

    bool init(System *sys) override
    {
        this->sys = sys;
        WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info)
                     { this->onWiFiEvent(event, info); });
        WiFi.setAutoReconnect(false);
        return true;
    }
    void update() override
    {
        if (state == WiFiConnectionState::DISCONNECTED &&
            hasIntervalElapsed() &&
            autoReconnect)
            commence();
    }

    uint32_t updateInterval() override { return 10; }

    bool setSta(const char *ssid, const char *pass)
    {
        if (!areCredentialsValid(ssid, pass))
        {
            LOGF(sys, SRC_WIFI, LOG_ERROR, LOG_COLOR_RED,
                 "Invalid credentials for STA: SSID:%s, Password:%s\n", ssid, pass);
            return false;
        }

        if (strcmp(appliedConfig.staSsid, ssid) != 0)
        {
            strncpy(config.staSsid, ssid, WIFI_SSID_MAX_LEN);
            config.staSsid[WIFI_SSID_MAX_LEN] = '\0';
        }
        else
            LOG_INFO(sys, "STA SSID didn't change!", SRC_WIFI, LOG_COLOR_CYAN);

            if (pass && strlen(pass) > 0 && strcmp(appliedConfig.staPass, pass) != 0)
            {
                strncpy(config.staPass, pass, WIFI_PASS_MAX_LEN);
                config.staPass[WIFI_PASS_MAX_LEN] = '\0';
            }
            else
                LOG_INFO(sys, "STA password didn't change!", SRC_WIFI, LOG_COLOR_CYAN);

                return true;
    }
    bool setAp(const char *ssid, const char *pass)
    {
        if (!areCredentialsValid(ssid, pass))
        {
            LOGF(sys, SRC_WIFI, LOG_ERROR, LOG_COLOR_RED,
                 "Invalid credentials for AP: SSID:%s, Password:%s\n", ssid, pass);
            return false;
        }

        if (strcmp(appliedConfig.apSsid, ssid) != 0)
        {
            strncpy(config.apSsid, ssid, WIFI_SSID_MAX_LEN);
            config.apSsid[WIFI_SSID_MAX_LEN] = '\0';
        }
        else
            LOG_INFO(sys, "AP SSID didn't change!", SRC_WIFI, LOG_COLOR_CYAN);

            if (pass && strlen(pass) > 0 && strcmp(appliedConfig.apPass, pass) != 0)
            {
                strncpy(config.apPass, pass, WIFI_PASS_MAX_LEN);
                config.apPass[WIFI_PASS_MAX_LEN] = '\0';
            }
            else
                LOG_INFO(sys, "AP password didn't change!", SRC_WIFI, LOG_COLOR_CYAN);

                return true;
    }
    bool setMode(WiFiMode mode)
    {
        if (!isModeValid(mode))
        {
            LOGF(sys, SRC_WIFI, LOG_ERROR, LOG_COLOR_RED, "WiFi mode invalid: %d\n", mode);
            return false;
        }

        if (mode == appliedConfig.mode)
        {
            LOG_INFO(sys, "WiFi mode didn't change!", SRC_WIFI, LOG_COLOR_CYAN);
            return true;
        }

        config.mode = mode;
        return true;
    }
    inline void setAutoReconnect(bool enable) { autoReconnect = enable; }

    bool commence()
    {
        // Nothing changed & still connected
        if (config == appliedConfig &&
            state != WiFiConnectionState::DISCONNECTED)
        {
            LOG_INFO(sys, "No changes were made to WiFi!", SRC_WIFI, LOG_COLOR_CYAN);
            return true;
        }

        // Getting the new mode
        wifi_mode_t newMode = WIFI_OFF;

        switch (config.mode)
        {
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
        if (modeChanged)
            if (!WiFi.mode(newMode))
            {
                LOG_ERROR(sys, "Failed to change WiFi mode", SRC_WIFI);
                return false;
            }
            else
                LOG_DEBUG(sys, "WiFi mode changed!", SRC_WIFI);

        // Reconfigure STA if needed
        bool staChanged =
            strcmp(config.staSsid, appliedConfig.staSsid) != 0 ||
            strcmp(config.staPass, appliedConfig.staPass) != 0;

        if ((config.mode == WiFiMode::STA ||
             config.mode == WiFiMode::AP_STA) &&
                (staChanged ||
                 modeChanged) ||
            state == WiFiConnectionState::DISCONNECTED)
        {
            if (!WiFi.disconnect(false, false))
            {
                LOG_ERROR(sys, "Failed to disconnect STA", SRC_WIFI);
                return false;
            }
            else if (state != WiFiConnectionState::DISCONNECTED)
            {
                staIp = IPAddress();
                LOG_DEBUG(sys, "STA disconnected!", SRC_WIFI);
            }

            if (config.hasStaCred())
            {
                WiFi.begin(config.staSsid, config.staPass);
                state = WiFiConnectionState::CONNECTING;
                LOG_DEBUG(sys, "STA connecting...", SRC_WIFI);
            }
        }

        // Reconfigure AP if needed

        bool apChanged =
            strcmp(config.apSsid, appliedConfig.apSsid) != 0 ||
            strcmp(config.apPass, appliedConfig.apPass) != 0;

        if ((config.mode == WiFiMode::AP ||
             config.mode == WiFiMode::AP_STA) &&
            (apChanged ||
             modeChanged))
        {
            if (!WiFi.softAPdisconnect(true))
            {
                LOG_ERROR(sys, "Failed to disable AP", SRC_WIFI);
                return false;
            }
            else
            {
                apIp = IPAddress();
                LOG_DEBUG(sys, "AP disabled!", SRC_WIFI);
            }

            if (config.hasApCred())
                if (!WiFi.softAP(config.apSsid, config.apPass))
                {
                    LOG_ERROR(sys, "Failed to enable AP", SRC_WIFI);
                    return false;
                }
                else
                {
                    apIp = WiFi.softAPIP();
                    LOG_DEBUG(sys, "Enabled AP!", SRC_WIFI);
                }
        }

        // Save
        appliedConfig = config;
        return true;
    }

    bool stop()
    {

        if (!WiFi.disconnect(false, false))
        {
            LOG_ERROR(sys, "Failed to disconnect STA", SRC_WIFI);
            return false;
        }
        else
        {
            staIp = IPAddress();
            LOG_DEBUG(sys, "Disconnected STA!", SRC_WIFI);
        }

        if (!WiFi.softAPdisconnect(true))
        {
            LOG_ERROR(sys, "Failed to disable AP", SRC_WIFI);
            return false;
        }
        else
        {
            apIp = IPAddress();
            LOG_DEBUG(sys, "Disconnected STA!", SRC_WIFI);
        }

        if (!WiFi.mode(WIFI_OFF))
        {
            LOG_ERROR(sys, "Failed to turn off the WiFi", SRC_WIFI);
            return false;
        }
        else
            LOG_DEBUG(sys, "WiFi turned off!", SRC_WIFI);

        state = WiFiConnectionState::OFF;
        return true;
    }

    void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info)
    {
        switch (event)
        {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            state = WiFiConnectionState::CONNECTED;
            staIp = WiFi.localIP();
            EVENT_WIFI_CONNECTED(SRC_WIFI);
            LOG_INFO(sys, "WiFi Connected!", SRC_WIFI, LOG_COLOR_CYAN);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (state == WiFiConnectionState::OFF || state == WiFiConnectionState::CONNECTING)
                break;
            state = WiFiConnectionState::DISCONNECTED;
            staIp = IPAddress();
            EVENT_WIFI_DISCONNECTED(SRC_WIFI);
            LOG_WARN(sys, "WiFi Disconnected!", SRC_WIFI);
            break;
        }
    }

    inline WiFiConnectionState getConnectionState() const { return state; }
    inline bool isConnected() const { return state == WiFiConnectionState::CONNECTED; }
    inline WiFiMode getMode() const { return appliedConfig.mode; }
    inline uint8_t getClientCount() { return clientCount; }
    inline IPAddress getStaIp() const { return staIp; }
    inline IPAddress getApIp() const { return apIp; }

private:
    // =============== VARIABLES ===============
    System *sys;

    WiFiConnectionState state = WiFiConnectionState::OFF;

    IPAddress staIp;
    IPAddress apIp;

    bool autoReconnect = true;
    uint32_t reconnctInterval = 10 * 1000;
    uint32_t lastConnectionAttempt = 0;

    uint8_t clientCount = 0;
    bool hasClientCountChanged = false;

    WiFiConfig config;
    WiFiConfig appliedConfig;

    // =============== COMMANDS ===============
    static CommandResult setSta(void *ctx, const Command &cmd)
    {
        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount < 2)
            return {false, "Missing arguments: SSID and password"};

        if (!wifi->setSta(cmd.arg(0), cmd.arg(1)))
            return {false, "Invalid credentials. Check length requirements."};

        return {true, "AP credentials set successfully"};
    }

    static CommandResult setAp(void *ctx, const Command &cmd)
    {
        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount < 2)
            return {false, "Missing arguments: SSID and password"};

        if (!wifi->setAp(cmd.arg(0), cmd.arg(1)))
            return {false, "Invalid credentials. Check length requirements."};

        return {true, "AP credentials set successfully"};
    }

    static CommandResult setMode(void *ctx, const Command &cmd)
    {
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

    static CommandResult setAutoReconnect(void *ctx, const Command &cmd)
    {
        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: autoReconnect"};

        uint8_t ar = atoi(cmd.arg(0));
        if (ar > 1)
            return {false, "Invalid value. Must be 0 (OFF), or 1 (ON)"};

        wifi->setAutoReconnect(ar);
        return {true, "Auto reconnect changed!"};
    }

    static CommandResult commence(void *ctx, const Command &cmd)
    {
        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount)
            LOG_WARN(wifi->sys, "WiFi commence doesn't accept any arguments! Arguments ignored...", SRC_WIFI);

        bool success = wifi->commence();
        return {success, success ? "WiFi successfully started!" : "Failed to start WiFi"};
    }

    static CommandResult stop(void *ctx, const Command &cmd)
    {
        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);

        if (cmd.argumentCount)
            LOG_WARN(wifi->sys, "WiFi stop doesn't accept any arguments! Arguments ignored...", SRC_WIFI);

        bool success = wifi->stop();
        return {success, success ? "WiFi successfully stopped!" : "Failed to stop WiFi"};
    }

    static CommandResult getClientCount(void *ctx, const Command &cmd)
    {
        MyWiFiClient *wifi = static_cast<MyWiFiClient *>(ctx);
        LOGF(wifi->sys, SRC_WIFI, LOG_INFO, LOG_COLOR_MAGENTA, "AP client count: %d\n", wifi->getClientCount());
        return {true, "Success"};
    }

    static constexpr ModuleCommand moduleCommands[] = {
        {"setSta", "Set WiFi STA credentials <SSID> [password=\"\"]", setSta},
        {"setAp", "Set WiFi AP credentials <SSID> [password=\"\"]", setAp},
        {"setMode", "Set WiFi mode <0=STA|1=AP|2=AP+STA>", setMode},
        {"setAutoReconnect", "Set WiFi auto reconnect <0=OFF|1=ON>", setAutoReconnect},
        {"commence", "Commence WiFi network", commence},
        {"stop", "Stop WiFi module", stop},
        {"clientCount", "Get AP client count", getClientCount},
    };

    // =============== FUNCTIONS ===============
    inline bool hasIntervalElapsed() { return millis() - lastConnectionAttempt > reconnctInterval; }

    bool areCredentialsValid(const char *ssid, const char *pass)
    {
        if (strlen(ssid) == 0 || strlen(ssid) > WIFI_SSID_MAX_LEN)
            return false;
        if (pass &&
            (strlen(pass) > WIFI_PASS_MAX_LEN ||
             strlen(pass) < WIFI_PASS_MIN_LEN))
            return false;

        return true;
    }

    bool isModeValid(WiFiMode mode)
    {
        switch (mode)
        {
        case WiFiMode::STA:
        case WiFiMode::AP:
        case WiFiMode::AP_STA:
            return true;
        default:
            return false;
        }
    }
};
