#pragma once

// ---------- LOG LEVEL ----------
enum LogLevel : uint8_t
{
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
};

inline const char *toString(LogLevel l)
{
    switch (l)
    {
    case LOG_DEBUG:
        return "DEBUG";
    case LOG_WARN:
        return "WARN";
    case LOG_ERROR:
        return "ERROR";
    default:
        return "INFO";
    }
}

// ---------- LOG COLOR ----------

enum LogColor : uint8_t
{
    LOG_COLOR_BLACK,
    LOG_COLOR_RED,
    LOG_COLOR_GREEN,
    LOG_COLOR_YELLOW,
    LOG_COLOR_BLUE,
    LOG_COLOR_MAGENTA,
    LOG_COLOR_CYAN,
    LOG_COLOR_WHITE
};

static constexpr const char *toAnsi(LogColor c)
{
    switch (c)
    {
    case LOG_COLOR_BLACK:
        return "\033[30m";

    case LOG_COLOR_RED:
        return "\033[31m";

    case LOG_COLOR_GREEN:
        return "\033[32m";

    case LOG_COLOR_YELLOW:
        return "\033[33m";

    case LOG_COLOR_BLUE:
        return "\033[34m";

    case LOG_COLOR_MAGENTA:
        return "\033[35m";

    case LOG_COLOR_CYAN:
        return "\033[36m";

    case LOG_COLOR_WHITE:
        return "\033[37m";

    default:
        return "\033[0m";
    }
}
