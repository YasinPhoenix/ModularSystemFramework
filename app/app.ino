#include <Arduino.h>
#include "../core/System.h"
#include "../modules/Modules.h"
#include "Creds.h"

System sys;

TCPClient tcp;
MyWiFiClient wifi;
SerialModule serial;
LittleFsModule lfs;


void setup()
{
    serial.setLogLevel(LOG_DEBUG);
    serial.setColorUse(false);
    sys.addModule(&serial);

    sys.addModule(&lfs);

    // wifi.config(WIFI_CLIENT_MODE_STA, WIFI_SSID, WIFI_PASS);
    sys.addModule(&wifi);
    
    // tcp.setServer(TCP_SERVER_IP, TCP_SERVER_PORT);
    sys.addModule(&tcp);

    sys.start(); // starts Core 1 task (update processing)
}

void loop()
{
    // static uint32_t lastLog = 0;
    // if (millis() - lastLog > 5000)
    // {
    //     LOG(sys, "Main loop", SRC_APP, LOG_DEBUG, LOG_COLOR_MAGENTA);
    //     lastLog = millis();
    // }
    
    // Core 0 = event processing
    sys.processEvents();

    delay(1);
}