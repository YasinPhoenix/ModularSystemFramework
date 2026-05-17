#pragma once
#include "event.h"
#include "module_capability.h"

class IModule
{
public:
    virtual const char *name() = 0;
    virtual ModuleCapability capabilities() = 0;

    virtual bool init() = 0;
    virtual void update() = 0;

    // Declare interest
    virtual uint32_t eventMask() { return 0xFFFFFFFF; }
    // Default: listen to everyone (safe fallback)

    // Update interval in ms
    virtual uint32_t updateInterval() { return 1000; }

    virtual void onEvent(const Event &e) {}
};