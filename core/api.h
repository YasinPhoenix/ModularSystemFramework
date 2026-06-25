#pragma once
#include "system.h"
#include "event/event.h"

// ---------- Serial ----------
#define LOG(sys, msg, src, lvl, clr) \
    sys->emit(makeLogEvent(msg, src, lvl, clr));

#define LOGF(sys, src, lvl, clr, fmt, ...) \
    sys->emit(makeLogEvent(src, lvl, clr, fmt, __VA_ARGS__));

#define LOG_INFO(sys, msg, src) \
    sys->emit(makeLogEvent(msg, src, LOG_INFO));

#define LOG_INFO(sys, msg, src, clr) \
    sys->emit(makeLogEvent(msg, src, LOG_INFO, clr));

#define LOG_WARN(sys, msg, src) \
    sys->emit(makeLogEvent(msg, src, LOG_WARN, LOG_COLOR_YELLOW));

#define LOG_ERROR(sys, msg, src) \
    sys->emit(makeLogEvent(msg, src, LOG_ERROR, LOG_COLOR_RED));

#define LOG_DEBUG(sys, msg, src) \
    sys->emit(makeLogEvent(msg, src, LOG_DEBUG, LOG_COLOR_CYAN));

// ---------- TCP ----------

#define TCP_SEND(sys, key, value) \
    sys->emit(makeTCPSendEvent(key, value));

// ---------- WiFi ----------

#define EVENT_WIFI_CONNECTED(src) \
    makeWiFiEvent(true, src);
#define EVENT_WIFI_DISCONNECTED(src) \
    makeWiFiEvent(false, src);