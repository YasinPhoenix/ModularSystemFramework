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
    logger.setColorUse(true);
    sys.addModule(&logger);

    wifi.config(WIFI_MODULE_MODE_STA, WIFI_SSID, WIFI_PASS);
    sys.addModule(&wifi);

    tcp.setServer(TCP_SERVER_IP, TCP_SERVER_PORT);
    sys.addModule(&tcp);

    sys.start(); // starts Core 1 task (update processing)
}

void loop()
{
    // Core 0 = event processing
    sys.processEvents();

    delay(1);
}