#pragma once

#define COMMAND_NAME_MAX_LENGTH 32
#define COMMAND_ARGUMENT_MAX_LENGTH 64
#define COMMAND_MAX_ARGUMENTS 8

struct Command
{
    char name[COMMAND_NAME_MAX_LENGTH];
    char arguments[COMMAND_MAX_ARGUMENTS][COMMAND_ARGUMENT_MAX_LENGTH];
    uint8_t argumentCount = 0;

    const char *arg(uint8_t i) const
    {
        if (i >= argumentCount)
            return nullptr;

        return arguments[i];
    }
};

struct CommandResult
{
    bool success;
    char message[128];
};

typedef CommandResult (*CommandHandler)(void *context, const Command &);

struct CommandEntry
{
    const char *name;
    const char *help;
    CommandHandler handler;
    void *context;
};