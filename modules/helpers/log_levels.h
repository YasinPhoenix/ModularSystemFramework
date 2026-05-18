#pragma once

static constexpr const char *L_INFO = "INFO";
static constexpr const char *L_WARN = "WARN";
static constexpr const char *L_ERROR = "ERROR";
static constexpr const char *L_DEBUG = "DEBUG";

enum LogLevel
{
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
};

static constexpr const char *getLogLevel(LogLevel l)
{
    switch (l)
    {
    case LOG_WARN:
        return L_WARN;

    case LOG_ERROR:
        return L_ERROR;

    case LOG_DEBUG:
        return L_DEBUG;

    default:
        return L_INFO;
    }
}