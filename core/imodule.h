#pragma once
#include "event.h"

class IModule
{
public:
    virtual const char *name() = 0;
    virtual uint32_t capabilities() = 0;

    virtual bool init() = 0;
    virtual void update() = 0;

    // Declare interest
    virtual uint32_t eventMask() { return 0xFFFFFFFF; }
    // Default: listen to everyone (safe fallback)

    virtual void onEvent(const Event &e) {}
};