#pragma once
#include <WiFi.h>
#include "../core/imodule.h"
#include "../core/system.h"
#include "../core/api.h"
#include "helpers/wifi_helpers.h"

extern System sys;

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
        this->ssid = ssid;
        this->pass = pass;
        if (apSsid)
            this->apSsid = apSsid;
        if (apPass)
            this->apPass = apPass;
        if (apSsid)
            hasApCred = true;
        hasStaCred = true;
    }

    const char *name() override { return "WiFi"; }
    uint16_t capabilities() override { return CAPABILITY_BIT(CAPABILITY_WIFI); }

    bool init() override
    {
        setMode(mode);

        if (!connect())
            return false;

        if (mode == WIFI_MODULE_MODE_AP)
            state = WIFI_IDLE;
        else
            state = WIFI_CONNECTING;

        return true;
    }

    void update() override
    {
        uint32_t now = millis();

        if (mode == WIFI_MODULE_MODE_AP || mode == WIFI_MODULE_MODE_AP_STA)
        {
            clientCount = WiFi.AP.stationCount();
            if (now - lastApStaCountLog > apStaLogInterval)
            {
                sys.emit(makeLogEvent(SRC_WIFI, LOG_INFO, LOG_COLOR_WHITE, "AP stations: %d", clientCount));
                lastApStaCountLog = now;
            }
            if (mode == WIFI_MODULE_MODE_AP)
                return;
        }

        switch (state)
        {
        case WIFI_CONNECTING:
        {
            if (WiFi.isConnected())
            {
                state = WIFI_CONNECTED;
                EVENT_WIFI_CONNECTED(SRC_WIFI);
                LOG_INFO(sys, "WiFi Connected!", SRC_WIFI);
            }
            break;
        }
        case WIFI_CONNECTED:
        {
            if (!WiFi.isConnected() && state != WIFI_CONNECTING)
            {
                state = WIFI_DISCONNECTED;
                disconnectionTime = now;
                EVENT_WIFI_DISCONNECTED(SRC_WIFI);
                LOG_INFO(sys, "WiFi Disconnected!", SRC_WIFI);
            }
            break;
        }
        case WIFI_DISCONNECTED:
        {
            if (connect())
            {
                state = WIFI_CONNECTING;
                LOG_INFO(sys, "WiFi reconnecting!", SRC_WIFI);
            }

            break;
        }
        default:
            LOG_WARN(sys, "Unexpected state in wifi_module update!", SRC_WIFI);
            break;
        }
    }

    // uint32_t eventMask() override { return EVENT_BIT(EVENT_SENSOR_UPDATE); }
    uint32_t updateInterval() override { return 10; }

    void onEvent(const Event &e) override
    {
    }

    bool setStaCred(const char *ssid, const char *pass)
    {
        if (strlen(ssid) > WIFI_SSID_MAX_LEN ||
            strlen(pass) > WIFI_PASS_MAX_LEN ||
            strlen(pass) < WIFI_PASS_MIN_LEN)
            return false;

        this->ssid = ssid;
        this->pass = pass;
        hasStaCred = true;
        return true;
    }

    bool setApCred(const char *ssid, const char *pass)
    {
        if (strlen(ssid) > WIFI_SSID_MAX_LEN ||
            strlen(pass) > WIFI_PASS_MAX_LEN ||
            strlen(pass) < WIFI_PASS_MIN_LEN)
            return false;

        apSsid = ssid;
        apPass = pass;
        hasApCred = true;
        return true;
    }

    bool setMode(WiFiModeState mode)
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

    uint32_t apStaLogInterval = 10 * 1000;

    IPAddress ip;

    uint8_t clientCount = 0;

    const char *ssid;
    const char *pass;
    bool hasStaCred = false;

    const char *apSsid;
    const char *apPass;
    bool hasApCred = false;

    bool connect(bool ignoreTimer = false;)
    {
        if (millis() - disconnectionTime < reconnectTimer)
            return false;

        switch (mode)
        {
        case WIFI_MODULE_MODE_STA:
        {
            if (!hasStaCred)
                return false;
            WiFi.begin(ssid, pass);
            return true;
        }
        case WIFI_MODULE_MODE_AP:
            return startAp();

        case WIFI_MODULE_MODE_AP_STA:
        {
            if (!hasStaCred || !startAp())
                return false;
            WiFi.begin(ssid, pass);
            return true;
        }
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

    inline bool stopAp() { return WiFi.AP.end(); }
    inline bool disconnectSta() { return WiFi.disconnect(); }
};