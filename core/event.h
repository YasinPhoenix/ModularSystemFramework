#pragma once
#include "event_types.h"
#include "event_priorities.h"
#include "event_data_types.h"

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

static Event makeLogEvent(const char *msg,
                          uint16_t source,
                          LogLevel level = LOG_INFO,
                          LogColor color = LOG_COLOR_WHITE,
                          bool isBigMsg = false)
{
    Event e;
    e.sourceId = source;
    e.timestamp = millis();
    
    if (isBigMsg)
    {
        e.type = EVENT_LOG_BIG;
        auto &bigLog = e.data.bigLog;
        bigLog.level = level;
        bigLog.color = color;
        strncpy(bigLog.message, msg, LOG_MESSAGE_SIZE_BIG - 1);
        bigLog.message[LOG_MESSAGE_SIZE_BIG - 1] = '\0';
    }
    else
    {
        e.type = EVENT_LOG;
        auto &log = e.data.log;
        log.level = level;
        log.color = color;
        strncpy(log.message, msg, LOG_MESSAGE_SIZE - 1);
        log.message[LOG_MESSAGE_SIZE - 1] = '\0';
    }

    return e;
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