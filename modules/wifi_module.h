#pragma once
#include "../core/imodule.h"

class WifiModule : public IModule
{
public:
    const char *name() override { return "WiFi"; }
    uint32_t capabilities() override { return 1 << 0; }

    bool init() override
    {
        return connect();
    }

    void update() override {}

    uint32_t eventMask() override
    {
        return EVENT_BIT(EVENT_SENSOR_UPDATE);
    }

    uint32_t updateInterval() override { return 10; }

    void onEvent(const Event &e) override
    {
        if (e.type == EVENT_SENSOR_UPDATE)
        {
            float temp = e.data.sensor.value;
            send(temp);
        }
    }

private:
    bool connect() { return true; }

    void send(float t)
    {
        // simulate send
    }
};