#pragma once
#include "../system.h"
#include "event_types.h"
#include "event_source.h"
#include "event_priorities.h"
#include "event_data_types.h"

struct Event
{
    EventType type;
    EventSource sourceId;
    uint32_t timestamp;

    EventData data;
};

// You're going to need to add event makers for each kind of event that you add in your project here
// or you can make them manually everywhere you have them emitted but this is a cleaner approach for
// bigger projects.

static Event makeLogEvent(
    const char *msg,
    EventSource source,
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
    EventSource source,
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

static Event makeTCPLogEvent(
    LogLevel level,
    const char *message)
{
    Event e;
    e.type = EVENT_TCP_SEND;
    e.sourceId = SRC_TCP;
    e.timestamp = millis();

    auto &tcpLog = e.data.tcpLog;

    tcpLog.level = level;

    strncpy(tcpLog.message, message, 128 - 1);
    tcpLog.message[128 - 1] = '\0';

    return e;
}

static Event makeWiFiEvent(bool connected, EventSource source)
{
    Event e;
    if (connected)
        e.type = EVENT_WIFI_CONNECTED;
    else
        e.type = EVENT_WIFI_DISCONNECTED;

    e.sourceId = source;
    e.timestamp = millis();
    
    return e;
}