#include <Arduino.h>
#include "../core/system.h"
#include "../modules/modules.h"
#include "creds.h"

System sys;

TCPClient tcp;
WifiModule wifi;
LoggerModule logger;

void setup()
{
    logger.setLogLevel(LOG_DEBUG);
    logger.setColorUse(false);
    sys.addModule(&logger);

    wifi.config(WIFI_MODULE_MODE_STA, WIFI_SSID, WIFI_PASS);
    sys.addModule(&wifi);

    sys.addModule(&tcp);

    sys.start(); // starts Core 1 task (update processing)
    tcp.setServer(TCP_SERVER_IP, TCP_SERVER_PORT);
}

void loop()
{
    static uint32_t lastLog = 0;
    if (millis() - lastLog > 5000)
    {
        LOG(sys, "Main loop", SRC_APP, LOG_DEBUG, LOG_COLOR_MAGENTA);
        lastLog = millis();
    }

    // Core 0 = event processing
    sys.processEvents();

    delay(1);
}