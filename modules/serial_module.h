#pragma once
#include "../core/imodule.h"
#include "../core/api.h"
#include "../core/event/event_source.h"
#include "helpers/log_helpers.h"
#include "../core/system.h"

class SerialModule : public IModule
{
public:
    const char *name() override { return "Serial"; }

    MODULE_COMMANDS();

    bool init(System *sys) override
    {
        this->sys = sys;
        Serial.begin(115200);
        return true;
    }

    void update() override
    {
        if (Serial.available())
        {
            char buffer[128];
            size_t len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';

            LOGF(sys, SRC_SERIAL, LOG_DEBUG, LOG_COLOR_MAGENTA, "Received command: %s", buffer);

            auto result = sys->executeCommand(buffer);
            if (result.success)
            {
                LOGF(sys, SRC_SERIAL, LOG_DEBUG, LOG_COLOR_GREEN, "Command executed successfully: %s", result.message);
            }
            else
            {
                LOGF(sys, SRC_SERIAL, LOG_ERROR, LOG_COLOR_RED, "Command execution failed: %s", result.message);
            }
        }
    }

    void setLogLevel(LogLevel l) { minLogLevel = l; }

    void setColorUse(bool b) { useColors = b; }

    uint32_t eventMask() override { return EVENT_BIT(EVENT_LOG); }

    void onEvent(const Event &e) override
    {
        const auto &log = e.data.log;

        if (log.level > minLogLevel)
            return;

        char buffer[160];

        uint32_t time = log.timeStamp / 1000;
        uint16_t hours = time / 3600;
        uint8_t minutes = (time % 3600) / 60;
        uint8_t seconds = (time % 60);

        snprintf(buffer, sizeof(buffer),
                 "[%d:%d:%d][%s][%s] %s",
                 hours,
                 minutes,
                 seconds,
                 toString(log.source),
                 toString(log.level),
                 log.message);

        if (useColors)
            Serial.print(toAnsi(log.color));

        Serial.println(buffer); // Serial output

        if (useColors)
            Serial.print("\033[0m");
    }

private:
    // =============== VARIABLES ===============
    System *sys;

    LogLevel minLogLevel = LOG_INFO;
    bool useColors = true;

    // =============== COMMANDS ===============
    static CommandResult setLogLevel(void *ctx, const Command &cmd)
    {
        SerialModule *serial = static_cast<SerialModule *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: log level"};

        int level = atoi(cmd.arg(0));
        if (level < 0 || level > 3)
            return {false, "Invalid log level. Must be between 0 and 3"};

        serial->setLogLevel(static_cast<LogLevel>(level));
        return {true, "Log level set successfully"};
    }

    static CommandResult setColorUse(void *ctx, const Command &cmd)
    {
        SerialModule *serial = static_cast<SerialModule *>(ctx);

        if (cmd.argumentCount < 1)
            return {false, "Missing argument: color use (0 or 1)"};

        int useColor = atoi(cmd.arg(0));
        if (useColor != 0 && useColor != 1)
            return {false, "Invalid argument. Must be 0 or 1"};

        serial->setColorUse(useColor == 1);
        return {true, "Color output setting updated successfully"};
    }

    static constexpr ModuleCommand moduleCommands[] = {
        {"setLogLevel", "Set the minimum log level <0=INFO|1=WARN|2=ERROR|3=DEBUG>", setLogLevel},
        {"setColorUse", "Enable or disable color output <enable=0>", setColorUse}};
};