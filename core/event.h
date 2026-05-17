#pragma once
#include "event_types.h"
#include "event_priority.h"

struct Event
{
    EventType type;
    uint32_t sourceId;
    uint32_t timestamp;

    EventData data;
};

static Event makeSensorEvent(float value, uint32_t source, uint32_t time)
{
    Event e;
    e.type = EVENT_SENSOR_UPDATE;
    e.sourceId = source;
    e.timestamp = time;
    e.data.sensor.value = value;
    return e;
}