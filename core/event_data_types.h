#pragma once
#include "../modules/helpers/log_types.h"

// Payloads (STRICT TYPES)

#define LOG_MESSAGE_SIZE 128

struct LogData{
    LogLevel level;
    LogColor color;
    uint16_t source;
    uint32_t timeStamp;
    char message[LOG_MESSAGE_SIZE];
};

struct SensorUpdateData
{
    float value;
};

struct WifiStatusData
{
    float value;
};

// Union of all possible payloads

union EventData
{
    LogData log;
    SensorUpdateData sensor;
    WifiStatusData wifi;
};
