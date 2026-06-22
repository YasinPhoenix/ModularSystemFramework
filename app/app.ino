#include <Arduino.h>
#include "../core/system.h"
#include "../modules/modules.h"
#include "creds.h"

System sys;

WifiModule wifi;
LoggerModule logger;

void setup()
{
    logger.setLogLevel(LOG_DEBUG);
    logger.setColorUse(true);
    logger.setWiFiLogger(WIFI_LOGGER_IP, WIFI_LOGGER_PORT);
    sys.addModule(&logger);

    wifi.config(WIFI_MODULE_MODE_STA, WIFI_SSID, WIFI_PASS);
    sys.addModule(&wifi);

    sys.start(); // starts Core 1 task (update processing)
}

void loop()
{
    // Core 0 = event processing
    sys.processEvents();

    delay(1);
}