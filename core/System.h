#pragma once
#include "module/ModuleRegistry.h"
#include "command/CommandRegistry.h"
#include "command/CommandParser.h"
#include "event/EventQueue.h"
#include "fileSystem/IFileSystem.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class System
{
public:
    System()
    {
        registerBuiltInCommands();
    }

private:
    ModuleRegistry registry;
    CommandRegistry commands;
    EventQueue highQueue;
    EventQueue normalQueue;
    EventQueue lowQueue;

    IFileSystem *fileSystem = nullptr;

    TaskHandle_t updateTaskHandle = nullptr;

    static void updateTask(void *arg)
    {
        System *sys = static_cast<System *>(arg);

        while (true)
        {
            uint32_t now = millis();

            for (int i = 0; i < sys->registry.size(); i++)
            {
                auto &entry = sys->registry.getEntry(i);
                IModule *m = entry.module;

                uint32_t interval = m->updateInterval();

                if (now - entry.lastUpdate >= interval)
                {
                    entry.lastUpdate = now;
                    m->update();
                }
            }

            vTaskDelay(1); // yield
        }
    }

public:
    bool addModule(IModule *m)
    {
        if (auto *fs = m->asFileSystem())
            fileSystem = fs;

        if (!registry.add(m, this))
            return false;

        // Register module commands
        const ModuleCommand *cmds = m->getCommands();
        uint8_t cmdCount = m->getCommandCount();

        for (uint8_t i = 0; i < cmdCount; i++)
        {
            const ModuleCommand &entry = cmds[i];
            if (!commands.registerCommand(m->name(),
                                          entry.name,
                                          entry.help,
                                          entry.handler,
                                          m))
            {
                return false; // Failed to register command
            }
        }

        return true;
    }

    IFileSystem *getFileSystem() { return fileSystem; }
    void setFileSystem(IFileSystem *fs) { fileSystem = fs; }

    static void emitLogLine(System *sys, const char *line)
    {
        sys->emit(makeLogEvent(line, SRC_SYSTEM, LOG_INFO, LOG_COLOR_MAGENTA));
    }

    static CommandResult helpHandler(void *context, const Command &cmd)
    {
        System *sys = static_cast<System *>(context);
        const char *moduleFilter = nullptr;
        const char *commandFilter = nullptr;

        if (cmd.argumentCount > 2)
            return {false, "Too many arguments. Usage: System --help [module] [command]"};

        if (cmd.argumentCount >= 1)
            moduleFilter = cmd.arg(0);

        if (cmd.argumentCount == 2)
            commandFilter = cmd.arg(1);

        const CommandEntry *entries = sys->commands.getEntries();
        uint8_t entryCount = sys->commands.getCount();
        char buffer[256];
        bool found = false;

        auto emitHeader = [&](const char *moduleName)
        {
            snprintf(buffer, sizeof(buffer), "  %s:", moduleName);
            emitLogLine(sys, buffer);
            emitLogLine(sys, "    Command             Help");
        };

        auto emitCommandRow = [&](const char *command, const char *help)
        {
            snprintf(buffer, sizeof(buffer), "    %-18s  %s",
                     command,
                     help ? help : "");
            emitLogLine(sys, buffer);
        };

        if (!moduleFilter)
        {
            emitLogLine(sys, "Available commands:");

            const char *modules[MAX_COMMANDS];
            uint8_t moduleCount = 0;

            for (uint8_t i = 0; i < entryCount; i++)
            {
                const char *moduleName = entries[i].moduleName;
                bool seen = false;

                for (uint8_t j = 0; j < moduleCount; j++)
                {
                    if (strcmp(modules[j], moduleName) == 0)
                    {
                        seen = true;
                        break;
                    }
                }

                if (!seen)
                    modules[moduleCount++] = moduleName;
            }

            for (uint8_t m = 0; m < moduleCount; m++)
            {
                emitHeader(modules[m]);
                for (uint8_t i = 0; i < entryCount; i++)
                {
                    if (strcmp(entries[i].moduleName, modules[m]) == 0)
                    {
                        char commandName[COMMAND_NAME_MAX_LENGTH + 3];
                        snprintf(commandName, sizeof(commandName), "--%s", entries[i].name);
                        emitCommandRow(commandName, entries[i].help);
                    }
                }
            }

            return {true, "Help output emitted"};
        }

        if (!commandFilter)
        {
            emitLogLine(sys, "Commands for module:");
            emitHeader(moduleFilter);

            for (uint8_t i = 0; i < entryCount; i++)
            {
                if (strcmp(entries[i].moduleName, moduleFilter) == 0)
                {
                    char commandName[COMMAND_NAME_MAX_LENGTH + 3];
                    snprintf(commandName, sizeof(commandName), "--%s", entries[i].name);
                    emitCommandRow(commandName, entries[i].help);
                    found = true;
                }
            }

            if (!found)
                return {false, "No commands found for module"};

            return {true, "Help output emitted"};
        }

        for (uint8_t i = 0; i < entryCount; i++)
        {
            if (strcmp(entries[i].moduleName, moduleFilter) == 0 &&
                strcmp(entries[i].name, commandFilter) == 0)
            {
                emitLogLine(sys, "Command detail:");
                emitHeader(moduleFilter);
                char commandName[COMMAND_NAME_MAX_LENGTH + 3];
                snprintf(commandName, sizeof(commandName), "--%s", entries[i].name);
                emitCommandRow(commandName, entries[i].help);
                return {true, "Help output emitted"};
            }
        }

        return {false, "Command not found"};
    }

    void registerBuiltInCommands()
    {
        commands.registerCommand("System",
                                 "help",
                                 "Show available commands. Usage: System --help [module] [command]",
                                 helpHandler,
                                 this);
    }

    CommandResult executeCommand(const char *cmd)
    {
        Command parsedCmd;
        CommandParseResult result = CommandParser::parse(cmd, parsedCmd);

        switch (result)
        {
        case CMD_PARSE_OK:
            return commands.execute(parsedCmd);

        case CMD_PARSE_EMPTY:
            return {false, "Empty command"};

        case CMD_PARSE_TOO_MANY_ARGS:
            return {false, "Too many arguments"};

        case CMD_PARSE_NAME_TOO_LONG:
            return {false, "Command name too long"};

        case CMD_PARSE_ARG_TOO_LONG:
            return {false, "Command argument too long"};

        case CMD_PARSE_INVALID_FORMAT:
            return {false, "Invalid command format"};

        case CMD_PARSE_MODULE_NAME_TOO_LONG:
            return {false, "Module name too long"};

        default:
            return {false, "Command parse error"};
        }
    }

    EventQueue &highEvents() { return highQueue; }
    EventQueue &normalEvents() { return normalQueue; }
    EventQueue &lowEvents() { return lowQueue; }

    void emit(const Event &e, EventPriority p = PRIORITY_NORMAL)
    {
        switch (p)
        {
        case PRIORITY_HIGH:
            highQueue.push(e);
            break;
        case PRIORITY_LOW:
            coalesceOrPush(lowQueue, e);
            break;
        default:
            normalQueue.push(e);
            break;
        }
    }

    void start()
    {
        xTaskCreatePinnedToCore(
            updateTask,
            "ModuleUpdateTask",
            4096,
            this,
            1,
            &updateTaskHandle,
            1 // Core 1
        );
    }

    void processEvents()
    {
        Event e;

        // 1. HIGH priority first
        while (highQueue.pop(e))
        {
            dispatch(e);
        }

        // 2. NORMAL
        while (normalQueue.pop(e))
        {
            dispatch(e);
        }

        // 3. LOW
        while (lowQueue.pop(e))
        {
            dispatch(e);
        }
    }

    void dispatch(const Event &e)
    {
        uint32_t bit = EVENT_BIT(e.type);

        for (int i = 0; i < registry.size(); i++)
        {
            IModule *m = registry.get(i);

            if (m->eventMask() & bit)
            {
                m->onEvent(e);
            }
        }
    }

    void coalesceOrPush(EventQueue &q, const Event &e)
    {
        // Try to replace existing event of same type
        if (replaceExisting(q, e))
        {
            return;
        }

        // Otherwise push normally
        q.push(e);
    }

    bool replaceExisting(EventQueue &q, const Event &e)
    {
        for (uint16_t i = 0; i < EVENT_QUEUE_SIZE; i++)
        {
            Event slot = q.getBufferIndex(i);

            if (slot.type == e.type)
            {
                slot = e; // overwrite with latest
                return true;
            }
        }

        return false;
    }
};