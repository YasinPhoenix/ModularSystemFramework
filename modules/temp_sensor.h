#pragma once
#include "../core/system.h"
#include "../core/imodule.h"

extern System sys;

class TempSensor : public IModule
{
public:
    const char *name() override { return "TempSensor"; }
    uint16_t capabilities() override { return CAPABILITY_BIT(CAPABILITY_INPUT); }

    bool init() override { return true; }

    void update() override
    {
        uint32_t now = millis();

        if (now - lastEmit < emitInterval)
        {
            return;
        }
        lastEmit = now;

        float temp = readTemp();

        sys.emit(makeSensorEvent(temp, SRC_SENSOR_TEMP, millis()), PRIORITY_NORMAL);
    }

    uint32_t eventMask() override { return 0; } // Listen to nothing

    uint32_t updateInterval() override { return 500; }

private:
    uint32_t lastEmit = 0;
    const uint32_t emitInterval = 500;

    float readTemp()
    {
        return 25.0 + (millis() % 1000) * 0.01;
    }
};