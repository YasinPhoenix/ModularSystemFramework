#pragma once
#include <WiFi.h>
#include "../core/imodule.h"
#include "../core/system.h"
#include "../core/api.h"
#include "helpers/wifi_helpers.h"

extern System sys;
static void wifiEvent(arduino_event_id_t event, arduino_event_info_t info);

class WifiModule : public IModule
{
public:
    void config(
        WiFiModeState mode,
        const char *ssid,
        const char *pass,
        const char *apSsid = 0,
        const char *apPass = 0)
    {
        this->mode = mode;
        strncpy(this->ssid, ssid, WIFI_SSID_MAX_LEN);
        this->ssid[WIFI_SSID_MAX_LEN] = '\0';

        strncpy(this->pass, pass, WIFI_PASS_MAX_LEN);
        this->pass[WIFI_PASS_MAX_LEN] = '\0';

        if (apSsid)
        {
            strncpy(this->apSsid, apSsid, WIFI_SSID_MAX_LEN);
            this->apSsid[WIFI_SSID_MAX_LEN] = '\0';
        }

        if (apPass)
        {
            strncpy(this->apPass, apPass, WIFI_PASS_MAX_LEN);
            this->apPass[WIFI_PASS_MAX_LEN] = '\0';
        }

        if (apSsid)
            hasApCred = true;
        hasStaCred = true;
    }

    const char *name() override { return "WiFi"; }
    uint16_t capabilities() override { return CAPABILITY_BIT(CAPABILITY_WIFI); }

    MODULE_COMMANDS();

    bool init() override
    {
        WiFi.onEvent(wifiEvent);
        setMode(mode);
        connect(true);
        return true;
    }

    void update() override
    {
        if (state == WIFI_DISCONNECTED)
            if (connect())
                LOG_INFO(sys, "WiFi reconnecting!", SRC_WIFI, LOG_COLOR_BLUE);
    }

    uint32_t updateInterval() override { return 10; }

    void onEvent(const Event &e) override {}

    bool setStaCred(const char *ssid, const char *pass)
    {
        if (strlen(ssid) > WIFI_SSID_MAX_LEN ||
            strlen(pass) > WIFI_PASS_MAX_LEN ||
            strlen(pass) < WIFI_PASS_MIN_LEN)
            return false;

        strncpy(this->ssid, ssid, WIFI_SSID_MAX_LEN);
        this->ssid[WIFI_SSID_MAX_LEN] = '\0';

        strncpy(this->pass, pass, WIFI_PASS_MAX_LEN);
        this->pass[WIFI_PASS_MAX_LEN] = '\0';
        hasStaCred = true;
        return true;
    }

    bool setApCred(const char *ssid, const char *pass)
    {
        if (strlen(ssid) > WIFI_SSID_MAX_LEN ||
            strlen(pass) > WIFI_PASS_MAX_LEN ||
            strlen(pass) < WIFI_PASS_MIN_LEN)
            return false;

        strncpy(this->apSsid, ssid, WIFI_SSID_MAX_LEN);
        this->apSsid[WIFI_SSID_MAX_LEN] = '\0';

        strncpy(this->apPass, pass, WIFI_PASS_MAX_LEN);
        this->apPass[WIFI_PASS_MAX_LEN] = '\0';

        hasApCred = true;
        return true;
    }

    bool setModeAndReconnect(WiFiModeState mode)
    {
        switch (this->mode)
        {
        case WIFI_MODULE_MODE_AP_STA:
        {
            if (!stopAp() || !disconnectSta())
                return false;
            break;
        }
        case WIFI_MODULE_MODE_AP:
        {
            if (!stopAp())
                return false;
            break;
        }
        case WIFI_MODULE_MODE_STA:
        {
            if (!disconnectSta())
                return false;
            break;
        }
        }

        switch (mode)
        {
        case WIFI_MODULE_MODE_AP_STA:
            WiFi.mode(WIFI_AP_STA);
            break;

        case WIFI_MODULE_MODE_AP:
            WiFi.mode(WIFI_AP);
            break;

        case WIFI_MODULE_MODE_STA:
            WiFi.mode(WIFI_STA);
            break;
        }

        this->mode = mode;
        return connect(true);
    }

    void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info)
    {
        switch (event)
        {
        case ARDUINO_EVENT_WIFI_STA_START:
            LOG_INFO(sys, "STA started!", SRC_WIFI, LOG_COLOR_BLUE);
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            state = WIFI_CONNECTED;
            EVENT_WIFI_CONNECTED(SRC_WIFI);
            LOG_INFO(sys, "WiFi Connected!", SRC_WIFI, LOG_COLOR_BLUE);
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ip = WiFi.STA.localIP();
            sys.emit(makeLogEvent(SRC_WIFI, LOG_INFO, LOG_COLOR_BLUE, "STA IP: %s", ip.toString().c_str()));
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (state == WIFI_DISCONNECTED)
                break;
            state = WIFI_DISCONNECTED;
            disconnectionTime = millis();
            EVENT_WIFI_DISCONNECTED(SRC_WIFI);
            LOG_INFO(sys, "WiFi Disconnected!", SRC_WIFI, LOG_COLOR_BLUE);
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            LOG_INFO(sys, "STA stopped!", SRC_WIFI, LOG_COLOR_BLUE);
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            LOG_INFO(sys, "AP started!", SRC_WIFI, LOG_COLOR_BLUE);
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            clientCount++;
            LOG_INFO(sys, "New AP client connected!", SRC_WIFI, LOG_COLOR_BLUE);
            sys.emit(makeLogEvent(SRC_WIFI, LOG_INFO, LOG_COLOR_BLUE, "AP stations: %d", clientCount));
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            sys.emit(makeLogEvent(SRC_WIFI, LOG_INFO, LOG_COLOR_BLUE,
                                  "AP STA IP assigned: %s",
                                  IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str()));
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            clientCount--;
            LOG_INFO(sys, "AP client disconnected!", SRC_WIFI, LOG_COLOR_BLUE);
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            LOG_INFO(sys, "AP stopped!", SRC_WIFI, LOG_COLOR_BLUE);
            break;
        }
    }

    inline WiFiConnectionState getConnectionState() const { return state; }

    inline bool isConnected() const { return state == WIFI_CONNECTED; }

    inline WiFiModeState getMode() const { return mode; }

    inline uint8_t getClientCount() { return clientCount; }

    inline IPAddress getIp() const { return ip; }

private:
    WiFiConnectionState state = WIFI_IDLE;
    WiFiModeState mode;

    uint32_t disconnectionTime = 0;
    uint32_t reconnectTimer = 10 * 1000; // 10 sec

    // uint32_t lastApStaCountLog = 0;
    // uint32_t apStaLogInterval = 10 * 1000;

    IPAddress ip;

    uint8_t clientCount = 0;

    char ssid[WIFI_SSID_MAX_LEN + 1];
    char pass[WIFI_PASS_MAX_LEN + 1];
    bool hasStaCred = false;

    char apSsid[WIFI_SSID_MAX_LEN + 1];
    char apPass[WIFI_PASS_MAX_LEN + 1];
    bool hasApCred = false;

    static constexpr ModuleCommand moduleCommands[] = {
        {"wifi.setStaCred", "Set the WiFi STA credentials (SSID and password)", [](void *context, const Command &cmd) -> CommandResult
         {
             WifiModule *wifi = static_cast<WifiModule *>(context);

             if (cmd.argumentCount < 2)
                 return {false, "Missing arguments: SSID and password"};

             if (!wifi->setStaCred(cmd.arg(0), cmd.arg(1)))
                 return {false, "Invalid credentials. Check length requirements."};

             return {true, "STA credentials set successfully"};
         }},
        {"wifi.setApCred", "Set the WiFi AP credentials (SSID and password)", [](void *context, const Command &cmd) -> CommandResult
         {
             WifiModule *wifi = static_cast<WifiModule *>(context);

             if (cmd.argumentCount < 2)
                 return {false, "Missing arguments: SSID and password"};

             if (!wifi->setApCred(cmd.arg(0), cmd.arg(1)))
                 return {false, "Invalid credentials. Check length requirements."};

             return {true, "AP credentials set successfully"};
         }},
        {"wifi.setMode", "Set the WiFi mode (0=STA, 1=AP, 2=AP+STA)", [](void *context, const Command &cmd) -> CommandResult
         {
             WifiModule *wifi = static_cast<WifiModule *>(context);

             if (cmd.argumentCount < 1)
                 return {false, "Missing argument: mode"};

             int mode = atoi(cmd.arg(0));
             if (mode < 0 || mode > 2)
                 return {false, "Invalid mode. Must be 0 (STA), 1 (AP), or 2 (AP+STA)"};

             if (!wifi->setModeAndReconnect(static_cast<WiFiModeState>(mode)))
                 return {false, "Failed to set mode and reconnect"};

             return {true, "WiFi mode set successfully"};
         }}};

    void setMode(WiFiModeState mode)
    {
        switch (mode)
        {
        case WIFI_MODULE_MODE_AP_STA:
            WiFi.mode(WIFI_AP_STA);
            break;

        case WIFI_MODULE_MODE_AP:
            WiFi.mode(WIFI_AP);
            break;

        case WIFI_MODULE_MODE_STA:
            WiFi.mode(WIFI_STA);
            break;

        default:
            LOG_WARN(sys, "Invalid WiFi mode!", SRC_WIFI);
            break;
        }
    }

    bool connect(bool ignoreTimer = false)
    {
        if (millis() - disconnectionTime < reconnectTimer && !ignoreTimer)
            return false;

        switch (mode)
        {
        case WIFI_MODULE_MODE_STA:
            if (!startSta())
                return false;
            state = WIFI_CONNECTING;
            return true;

        case WIFI_MODULE_MODE_AP:
            if (!startAp())
                return false;
            state = WIFI_AP_ONLY;
            return true;

        case WIFI_MODULE_MODE_AP_STA:
            if (!startSta() || !startAp())
                return false;
            state = WIFI_CONNECTING;
            return true;
        }
    }

    bool startAp()
    {
        if (!hasApCred)
            return false;
        WiFi.AP.begin();
        WiFi.AP.create(apSsid, apPass);
        if (!WiFi.AP.waitStatusBits(ESP_NETIF_STARTED_BIT, 1000))
            return false;
        return true;
    }

    bool startSta()
    {
        Serial.println("STA starting...");

        if (!hasStaCred)
            return false;

        WiFi.begin(ssid, pass);
        return true;
    }

    inline bool stopAp() { return WiFi.AP.end(); }
    inline bool disconnectSta() { return WiFi.disconnect(); }
};

extern WifiModule wifi;

static void wifiEvent(arduino_event_id_t event, arduino_event_info_t info)
{
    wifi.onWiFiEvent(event, info);
}