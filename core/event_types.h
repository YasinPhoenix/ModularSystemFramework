#pragma once
#include <stdint.h>

enum EventType {
    EVENT_NONE = 0,
    EVENT_SENSOR_UPDATE,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_MAX
};

// Convert event type -> bitmask

#define EVENT_BIT(e) (1 << (e))

// Payloads (STRICT TYPES)

struct SensorUpdateData{
    float value;
};

struct WifiStatusData{
    float value;
};

// Union of all possible payloads

union EventData {
    SensorUpdateData sensor;
    WifiStatusData wifi;
};
