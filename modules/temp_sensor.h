#pragma once
#include "../core/system.h"
#include "../core/imodule.h"

extern System sys;

class TempSensor : public IModule {
public:
    const char* name() override { return "TempSensor"; }
    uint32_t capabilities() override { return 1 << 1; }

    bool init() override { return true; }

    void update() override {
        float temp = readTemp();

        Event e;
        e.type = EVENT_SENSOR_UPDATE;
        e.sourceId = 1;
        e.timestamp = millis();

        e.data.sensor.value = temp;

        sys.emit(makeSensorEvent(temp, 1,millis()));
    }

    uint32_t eventMask() override {
        return 0; // Listen to nothing
    }

private:
    float readTemp() {
        return 25.0 + (millis() % 1000) * 0.01;
    }
};