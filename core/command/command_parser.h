#pragma once

#include <cstring>
#include "command.h"

enum CommandParseResult
{
    CMD_PARSE_OK,
    CMD_PARSE_EMPTY,
    CMD_PARSE_TOO_MANY_ARGS,
    CMD_PARSE_NAME_TOO_LONG,
    CMD_PARSE_ARG_TOO_LONG
};

class CommandParser
{
public:
    CommandParseResult parse(const char *str, Command &cmd)
    {
        memset(&cmd, 0, sizeof(cmd));

        if (!str)
            return CMD_PARSE_EMPTY;

        char buffer[128];
        strncpy(buffer, str, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        char *token = strtok(buffer, " ");

        if (!token)
            return CMD_PARSE_EMPTY;

        if (strlen(token) >= COMMAND_NAME_MAX_LENGTH)
            return CMD_PARSE_NAME_TOO_LONG;

        strncpy(cmd.name, token, COMMAND_NAME_MAX_LENGTH - 1);
        cmd.name[COMMAND_NAME_MAX_LENGTH - 1] = '\0';

        while (true)
        {
            token = strtok(nullptr, " ");

            if (!token)
                break;

            if (cmd.argumentCount >= COMMAND_MAX_ARGUMENTS)
                return CMD_PARSE_TOO_MANY_ARGS;

            if (strlen(token) >= COMMAND_ARGUMENT_MAX_LENGTH)
                return CMD_PARSE_ARG_TOO_LONG;

            strncpy(cmd.arguments[cmd.argumentCount], token, COMMAND_ARGUMENT_MAX_LENGTH - 1);

            cmd.arguments[cmd.argumentCount][COMMAND_ARGUMENT_MAX_LENGTH - 1] = '\0';

            cmd.argumentCount++;
        }

        return CMD_PARSE_OK;
    }
}