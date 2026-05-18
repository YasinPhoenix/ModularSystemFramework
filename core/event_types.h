#pragma once

// This is the part that you add your event types in order for the event subscribtion system to work.
enum EventType
{
    EVENT_NONE = 0,
    EVENT_LOG,
    EVENT_LOG_BIG,
    EVENT_SENSOR_UPDATE,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_DEFAULT
};

// Convert event type bitmask
#define EVENT_BIT(e) (1 << (e))
