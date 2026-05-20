#pragma once
#include "../core/imodule.h"
#include "../core/api.h"
#include "../core/event/event_source.h"
#include "helpers/log_helpers.h"
#include "helpers/wifi_logger.h"

class LoggerModule : public IModule
{
public:
    const char *name() override { return "Logger"; }

    bool init() override
    {
        Serial.begin(115200);

        if (wifiHost && wifiPort)
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

    uint32_t eventMask() override { return EVENT_BIT(EVENT_LOG); }
    void update() override { wifiLogger.update(); }

    void onEvent(const Event &e) override
    {
        const auto &log = e.data.log;

        if (log.level > minLogLevel)
            return;

        char buffer[160];

        uint32_t time = log.timeStamp / 1000;
        uint16_t hours = time / 3600;
        uint8_t minutes = (time % 3600) / 60;
        uint8_t seconds = (time % 60);

        snprintf(buffer, sizeof(buffer),
                 "[%d:%d:%d][SRC: %s][%s] %s",
                 hours,
                 minutes,
                 seconds,
                 toString(log.source),
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