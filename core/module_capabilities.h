#pragma once

// This is where the module capabilities are declared
enum ModuleCapability
{
    CAPABILITY_INPUT = 0,
    CAPABILITY_OUTPUT,
    CAPABILITY_WIFI
};

// Convert capability to bitmask
#define CAPABILITY_BIT(c) (1 << (c))