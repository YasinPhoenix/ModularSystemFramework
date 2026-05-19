#pragma once
#include "../core/imodule.h"
#include "helpers/log_types.h"
#include "helpers/wifi_logger.h"
#include "../core/api.h"

class LoggerModule : public IModule
{
public:
    bool init() override
    {
        Serial.begin(115200);

        if (!wifiHost || !wifiPort)
            return false;

        wifiLogger.begin(wifiHost, wifiPort);

        return true;
    }

    void setWiFiLogger(const char *host, uint16_t port)
    {
        wifiHost = host;
        wifiPort = port;
    }

    void setLogLevel(LogLevel l) { minLogLevel = l; }
    void setColorUse(bool b) { useColors = b; }

    uint32_t eventMask() override { EVENT_BIT(EVENT_LOG); }
    void update() override { wifiLogger.update(); }

    void onEvent(const Event &e) override
    {
        const auto &log = e.data.log;

        if (log.level < minLogLevel)
            return;

        char buffer[160];

        snprintf(buffer, sizeof(buffer),
                 "[%lu][SRC: %u][%s] %s",
                 log.timeStamp,
                 log.source,
                 toString(log.level),
                 log.message);

        if (useColors)
            Serial.print(toAnsi(log.color));

        Serial.println(buffer); // Serial output

        if (useColors)
            Serial.println("\033[0m");

        wifiLogger.log(buffer); // WiFi output
    }

private:
    LogLevel minLogLevel = LOG_INFO;
    bool useColors = true;

    WiFiLogger wifiLogger;

    const char *wifiHost;
    uint16_t wifiPort;
};