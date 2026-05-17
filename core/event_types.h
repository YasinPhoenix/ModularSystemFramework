#pragma once
#include <stdint.h>

// Well this is the part that you add your event types in order for the event subscribtion system to work.

enum EventType
{
    EVENT_NONE = 0,
    EVENT_SENSOR_UPDATE,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_DEFAULT
};

// Convert event type bitmask
#define EVENT_BIT(e) (1 << (e))

// Payloads (STRICT TYPES)

struct SensorUpdateData
{
    float value;
};

struct WifiStatusData
{
    float value;
};

struct DefaultData
{
    float value;
};

// Union of all possible payloads

union EventData
{
    SensorUpdateData sensor;
    WifiStatusData wifi;
    DefaultData defaultData;
};
