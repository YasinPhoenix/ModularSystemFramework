#pragma once

// This is the part that you add your event types in order for the event subscribtion system to work.
enum EventType
{
    EVENT_LOG,
    EVENT_TCP_SEND,
    EVENT_SENSOR_UPDATE,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED
};

// Convert event type bitmask
#define EVENT_BIT(e) (1 << (e))
