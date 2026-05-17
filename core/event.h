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

// You're going to need to add event makers for each kind of event that you add in your project here
// or you can make them manually everywhere you have them emitted but this is a cleaner approach for
// bigger projects.

static Event makeSensorEvent(float value, uint32_t source, uint32_t time)
{
    Event e;
    e.type = EVENT_SENSOR_UPDATE;
    e.sourceId = source;
    e.timestamp = time;
    e.data.sensor.value = value;
    return e;
}