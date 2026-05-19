#pragma once
#include "system.h"
#include "event.h"

// ---------- Logger ----------

#define LOG_INFO(sys, msg, src) \
    sys.emit(makeLogEvent(msg, src, LOG_INFO));

#define LOG_WARN(sys, msg, src) \
    sys.emit(makeLogEvent(msg, src, LOG_WARN, LOG_COLOR_YELLOW));

#define LOG_ERROR(sys, msg, src) \
    sys.emit(makeLogEvent(msg, src, LOG_ERROR, LOG_COLOR_RED));

#define LOG_DEBUG(sys, msg, src) \
    sys.emit(makeLogEvent(msg, src, LOG_DEBUG, LOG_COLOR_CYAN));