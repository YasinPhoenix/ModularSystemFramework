#include <Arduino.h>
#include "../core/system.h"
// #include "../modules/temp_sensor.h"
#include "../modules/wifi_module.h"
#include "../modules/logger_module.h"

System sys;

// TempSensor temp;
WifiModule wifi;
LoggerModule logger;

void setup()
{
    // sys.addModule(&temp);
    sys.addModule(&logger);
    sys.addModule(&wifi);

    logger.init();
    logger.setLogLevel(LOG_DEBUG);

    wifi.config(WIFI_MODULE_MODE_AP_STA, "OMEGA", "phoenix87", "ESP_AP_TEST", "12345678");
    wifi.init();

    sys.start(); // starts Core 1 task (update processing)

    sys.emit(makeLogEvent(SRC_APP, LOG_INFO, LOG_COLOR_WHITE, "Program started!"));
    LOG_INFO(sys, "Program Started!", SRC_APP);
}

void loop()
{
    // Core 0 = event processing
    sys.processEvents();

    delay(1);
}