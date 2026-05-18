#pragma once
#include "../core/imodule.h"

class LoggerModule : public IModule
{
public:
    bool init() override { Serial.begin(115200); }

    void onEvent(const Event &e) override
    {
        Serial.print(COL_GREEN);
        Serial.print("[");
        Serial.print(e.timestamp);
        Serial.print("]");
        Serial.print(COL_BLUE);
        Serial.print("[");
        Serial.print(e.sourceId);
        Serial.print("] ");

        switch (e.type)
        {
        case EVENT_LOG:
        {
            auto &log = e.data.log;
            Serial.print(getColorCode(log.color));
            Serial.print(getLogLevel(log.level));
            Serial.print(" ");
            Serial.print(log.message);
            break;
        }

        case EVENT_LOG_BIG:
        {
            auto &biglog = e.data.bigLog;
            Serial.print(getColorCode(biglog.color));
            Serial.print(getLogLevel(biglog.level));
            Serial.print(" ");
            Serial.print(biglog.message);
            break;
        }

        default:
            printEventCall(e);
            break;
        }

        Serial.println(COL_RESET);
    }

private:
    void printEventCall(const Event &e)
    {
        Serial.print(COL_YELLOW);
        Serial.print("Event called: ");
        Serial.print(e.type);
    }
};