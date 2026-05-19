#pragma once
#include "system.h"
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

static Event makeLogEvent(
    const char *msg,
    uint16_t source,
    LogLevel level = LOG_INFO,
    LogColor color = LOG_COLOR_WHITE)
{
    Event e;
    e.type = EVENT_LOG;
    e.sourceId = source;
    e.timestamp = millis();

    auto &log = e.data.log;

    log.level = level;
    log.color = color;
    log.source = source;
    log.timeStamp = e.timestamp;

    strncpy(log.message, msg, LOG_MESSAGE_SIZE - 1);
    log.message[LOG_MESSAGE_SIZE - 1] = '\0';

    return e;
}

static Event makeLogEvent(
    uint16_t source,
    LogLevel level,
    LogColor color,
    const char *format,
    ...)
{
    Event e;
    e.type = EVENT_LOG;
    e.sourceId = source;
    e.timestamp = millis();

    auto &log = e.data.log;

    log.level = level;
    log.color = color;
    log.source = source;
    log.timeStamp = e.timestamp;

    va_list args;
    va_start(args, format);
    vsnprintf(log.message, LOG_MESSAGE_SIZE, format, args);
    va_end(args);

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
