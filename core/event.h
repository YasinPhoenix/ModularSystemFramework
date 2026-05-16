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

static Event makeEvent(EventType type, uint32_t sourceId, uint32_t timeStamp, float value)
{
    Event e;
    e.type = type;
    e.sourceId = sourceId;
    e.timestamp = timeStamp;
    switch (type)
    {
    case EVENT_SENSOR_UPDATE:
        e.data.sensor.value = value;
        break;
    default:
        e.data.defaultData.value = value;
        break;
    }
}

static Event makeSensorEvent(float value, uint32_t source, uint32_t time)
{
    Event e;
    e.type = EVENT_SENSOR_UPDATE;
    e.sourceId = source;
    e.timestamp = time;
    e.data.sensor.value = value;
    return e;
}