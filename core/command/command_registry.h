#pragma once

#include "command.h"

#define MAX_COMMANDS 64

class CommandRegistry
{
public:
    bool registerCommand(
        const char *name,
        const char *help,
        CommandHandler handler,
        void *context = nullptr)
    {
        if (count >= MAX_COMMANDS || findCommand(name))
            return false;

        commands[count++] =
            {
                name,
                help,
                handler,
                context};

        return true;
    }

    CommandResult execute(const Command &cmd)
    {
        auto *entry = findCommand(cmd.name);

        if (!entry)
            return {false, "Unknown command"};

        return entry->handler(
            entry->context,
            cmd);
    }

private:
    CommandEntry commands[MAX_COMMANDS];
    uint8_t count = 0;

    CommandEntry *findCommand(const char *name)
    {
        for (uint8_t i = 0; i < count; i++)
        {
            if (strcmp(commands[i].name, name) == 0)
                return &commands[i];
        }

        return nullptr;
    }
};