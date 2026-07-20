#pragma once
#include "../core/module/IModule.h"
#include "../core/event/EventSource.h"
#include "../core/System.h"
#include "../core/API.h"
#include "common/LogCommon.h"

class SerialModule : public IModule
{
public:
    const char *name() override { return "Serial"; }

    MODULE_COMMANDS();

    bool init(System *sys) override
    {
        if (!sys)
        {
            LOG_ERROR(sys, "System wasn't given at module initiation!", SRC_WIFI);
            return false;
        }

        this->sys = sys;

        Serial.begin(115200);

        char result[128];
        configAvailable = scope.init(name(), sys->getFileSystem(), result);
        LOGF(sys, SRC_SERIAL, LOG_DEBUG, LOG_COLOR_MAGENTA, "Config scope initialization result: %s\n", result);

        loadConfig();

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

    void setLogLevel(LogLevel l)
    {
        minLogLevel = l;

        if (!scope.set("logLevel", toString(minLogLevel)))
            LOG_ERROR(sys, "Failed to save log level!", SRC_SERIAL);
    }

    void setColorUse(bool b)
    {
        useColors = b;

        if (!scope.set("colorUse", useColors ? "1" : "0"))
            LOG_ERROR(sys, "Failed to save color use option!", SRC_SERIAL);
    }

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
    LogLevel minLogLevel = LOG_DEBUG;
    bool useColors = true;

    ConfigScope scope;
    bool configAvailable = false;

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

    bool loadConfig()
    {
        if (!configAvailable)
            return false;

        char result[128];

        constexpr const char *keys[] = {"colorUse", "logLevel"};

        uint8_t index = 0;
        uint8_t availableCount = 0;

        for (const char *key : keys)
        {
            for (const char *item : scope.items)
            {
                if (item[0] == '\0') // skip empty entries
                    continue;

                if (strcmp(key, item) == 0)
                {
                    const char *value = scope.get(key);

                    switch (index)
                    {
                    case 0:
                        if (strcmp(value, "1") == 0)
                        {
                            useColors = true;
                        }
                        else if (strcmp(value, "0") == 0)
                        {
                            useColors = false;
                        }
                        else
                        {
                            LOGF(sys, SRC_SERIAL, LOG_ERROR, LOG_COLOR_RED, "Failed to load color use option, value: %s", value);
                            break;
                        }

                        LOGF(sys, SRC_SERIAL, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded colorUse from config: %u", useColors);
                        break;

                    case 1:
                        if (strcmp(value, "INFO") == 0)
                        {
                            minLogLevel = LOG_INFO;
                        }
                        else if (strcmp(value, "WARN") == 0)
                        {
                            minLogLevel = LOG_WARN;
                        }
                        else if (strcmp(value, "ERROR") == 0)
                        {
                            minLogLevel = LOG_ERROR;
                        }
                        else if (strcmp(value, "DEBUG") == 0)
                        {
                            minLogLevel = LOG_DEBUG;
                        }
                        else
                        {
                            LOGF(sys, SRC_SERIAL, LOG_ERROR, LOG_COLOR_RED, "Failed to load log level, value: %s", value);
                            break;
                        }

                        LOGF(sys, SRC_SERIAL, LOG_DEBUG, LOG_COLOR_CYAN, "Loaded logLevel from config: %s", toString(minLogLevel));
                        break;
                    }
                    availableCount++;
                    break;
                }
            }
            index++;
        }

        if (!availableCount)
            LOG_WARN(sys, "No serial configurations available!", SRC_SERIAL);

        return true;
    }
};