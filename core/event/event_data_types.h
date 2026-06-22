#pragma once
#include "../modules/helpers/log_helpers.h"

// Payloads (STRICT TYPES)

#define LOG_MESSAGE_SIZE 128

struct LogData
{
    LogLevel level;
    LogColor color;
    EventSource source;
    uint32_t timeStamp;
    char message[LOG_MESSAGE_SIZE];
};

struct TCPData
{
    char key[64];
    char value[128];
};

// Union of all possible payloads

union EventData
{
    LogData log;
    TCPData tcpData;
};
