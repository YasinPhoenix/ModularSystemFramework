#pragma once

#include "command.h"

#define MAX_COMMANDS 64

class CommandRegistry
{
public:
    bool registerCommand(
        const char *moduleName,
        const char *name,
        const char *help,
        CommandHandler handler,
        void *context = nullptr)
    {
        if (count >= MAX_COMMANDS || findCommand(moduleName, name))
            return false;

        commands[count++] =
            {
                moduleName,
                name,
                help,
                handler,
                context};

        return true;
    }

    CommandResult execute(const Command &cmd)
    {
        auto *entry = findCommand(cmd.moduleName, cmd.name);

        if (!entry)
            return {false, "Unknown command"};

        return entry->handler(
            entry->context,
            cmd);
    }

private:
    CommandEntry commands[MAX_COMMANDS];
    uint8_t count = 0;

    CommandEntry *findCommand(const char *moduleName, const char *name)
    {
        for (uint8_t i = 0; i < count; i++)
        {
            if (strcmp(commands[i].moduleName, moduleName) == 0 &&
                strcmp(commands[i].name, name) == 0)
                return &commands[i];
        }

        return nullptr;
    }
};