#include <Arduino.h>
#include "../core/system.h"
#include "../modules/temp_sensor.h"
#include "../modules/wifi_module.h"

System sys;

TempSensor temp;
WifiModule wifi;

void setup() {
    sys.addModule(&temp);
    sys.addModule(&wifi);

    sys.start(); // starts Core 1 task
}

void loop() {
    // Core 0 = event processing
    sys.processEvents();

    delay(1);
}