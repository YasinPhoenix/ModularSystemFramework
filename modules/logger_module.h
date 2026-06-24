#pragma once
#include "../core/imodule.h"
#include "../core/api.h"
#include "../core/event/event_source.h"
#include "helpers/log_helpers.h"

class LoggerModule : public IModule
{
public:
    const char *name() override { return "Logger"; }

    MODULE_COMMANDS();

    bool init() override
    {
        Serial.begin(115200);
        return true;
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
                 "[%d:%d:%d][SRC: %s][%s] %s",
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
            Serial.println("\033[0m");
    }

private:
    LogLevel minLogLevel = LOG_INFO;
    bool useColors = true;

    static constexpr ModuleCommand moduleCommands[] = {
        {"logger.setLogLevel", "Set the minimum log level (0=INFO, 1=WARN, 2=ERROR, 3=DEBUG)", [](void *context, const Command &cmd) -> CommandResult
         {
             LoggerModule *logger = static_cast<LoggerModule *>(context);

             if (cmd.argumentCount < 1)
                 return {false, "Missing argument: log level"};

             int level = atoi(cmd.arg(0));
             if (level < 0 || level > 3)
                 return {false, "Invalid log level. Must be between 0 and 3"};

             logger->setLogLevel(static_cast<LogLevel>(level));
             return {true, "Log level set successfully"};
         }},
        {"logger.setColorUse", "Enable or disable color output (0=disable, 1=enable)", [](void *context, const Command &cmd) -> CommandResult
         {
             LoggerModule *logger = static_cast<LoggerModule *>(context);

             if (cmd.argumentCount < 1)
                 return {false, "Missing argument: color use (0 or 1)"};

             int useColor = atoi(cmd.arg(0));
             if (useColor != 0 && useColor != 1)
                 return {false, "Invalid argument. Must be 0 or 1"};

             logger->setColorUse(useColor == 1);
             return {true, "Color output setting updated successfully"};
         }}};
};