#pragma once

enum class WiFiMode
{
    STA,
    AP,
    AP_STA,
    OFF
};

enum class WiFiConnectionState
{
    OFF,
    CONNECTED,
    CONNECTING,
    DISCONNECTED
};

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 32
#define WIFI_PASS_MIN_LEN 8

struct WiFiConfig
{
    WiFiMode mode = WiFiMode::OFF;

    char staSsid[WIFI_SSID_MAX_LEN + 1]{};
    char staPass[WIFI_PASS_MAX_LEN + 1]{};

    char apSsid[WIFI_SSID_MAX_LEN + 1]{};
    char apPass[WIFI_PASS_MAX_LEN + 1]{};

    bool hasStaCred() const { return staSsid[0] != '\0'; }
    bool hasApCred() const { return apSsid[0] != '\0'; }

    bool setMode(uint8_t mode)
    {
        switch (mode)
        {
        case 0:
            this->mode = WiFiMode::STA;
            return true;
        case 1:
            this->mode = WiFiMode::AP;
            return true;
        case 2:
            this->mode = WiFiMode::AP_STA;
            return true;
        default:
            this->mode = WiFiMode::OFF;
            return false;
        }
    }

    int8_t getMode() const
    {
        switch (mode)
        {
        case WiFiMode::STA:
            return 0;
        case WiFiMode::AP:
            return 1;
        case WiFiMode::AP_STA:
            return 2;
        default:
            return -1; // OFF
        }
    }

    const char *getModeStr()
    {
        switch (mode)
        {
        case WiFiMode::STA:
            return "0";
        case WiFiMode::AP:
            return "1";
        case WiFiMode::AP_STA:
            return "2";
        default:
            return "-1";
        }
    }

    bool operator==(const WiFiConfig &rhs) const
    {
        return mode == rhs.mode &&
               strcmp(staSsid, rhs.staSsid) == 0 &&
               strcmp(staPass, rhs.staPass) == 0 &&
               strcmp(apSsid, rhs.apSsid) == 0 &&
               strcmp(apPass, rhs.apPass) == 0;
    }

    bool operator!=(const WiFiConfig &rhs) const
    {
        return !(*this == rhs);
    }
};