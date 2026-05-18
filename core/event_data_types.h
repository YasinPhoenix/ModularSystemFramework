#pragma once
#include "../modules/helpers/log_colors.h"
#include "../modules/helpers/log_levels.h"

// Payloads (STRICT TYPES)

struct logDataWOT{
    LogLevel level;
    LogColor color;
};

#define LOG_MESSAGE_SIZE 64
#define LOG_MESSAGE_SIZE_BIG 256

struct LogData : logDataWOT{
    char message[LOG_MESSAGE_SIZE];
};

struct BigLogData : logDataWOT{
    char message[LOG_MESSAGE_SIZE_BIG];
};


struct SensorUpdateData
{
    float value;
};

struct WifiStatusData
{
    float value;
};

struct DefaultData
{
    float value;
};

// Union of all possible payloads

union EventData
{
    LogData log;
    BigLogData bigLog;
    SensorUpdateData sensor;
    WifiStatusData wifi;
    DefaultData defaultData;
};
