#include <Arduino.h>
#include "../core/System.h"
#include "../modules/Modules.h"
#include "Creds.h"
#include "Config.h"

System sys;

TCPClient tcp;
MyWiFiClient wifi(AppWiFiConfig);
SerialModule serial;
LittleFsModule lfs;


void setup()
{
    sys.addModule(&lfs);
    sys.addModule(&serial);
    sys.addModule(&wifi);
    sys.addModule(&tcp);

    sys.start(); // starts Core 1 task (update processing)
}

void loop()
{   
    // Core 0 = event processing
    sys.processEvents();

    delay(1);
}