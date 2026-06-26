#pragma once
#include "../event/Event.h"
#include "../command/Command.h"
#include "ModuleCaps.h"

class System;
class IFileSystem;

struct ModuleCommand
{
    const char *name;
    const char *help;
    CommandHandler handler;
};

class IModule
{
public:
    virtual const char *name() = 0;
    virtual uint16_t capabilities() { return 0xFFFFFFFF; }

    virtual const ModuleCommand *getCommands() { return nullptr; }
    virtual uint8_t getCommandCount() { return 0; }

    virtual bool init(System *sys) = 0;
    virtual void update() {};

    // Declare interest
    virtual uint32_t eventMask() { return 0xFFFFFFFF; }
    // Default: listen to everyone (safe fallback)

    // Update interval in ms
    virtual uint32_t updateInterval() { return 1000; }

    virtual void onEvent(const Event &e) {}

    virtual IFileSystem *asFileSystem() { return nullptr; }

private:
    System *sys;
};

// A little helper macro to define commands in a module
#define MODULE_COMMANDS(...)                                   \
    const ModuleCommand *getCommands() override                \
    {                                                          \
        return moduleCommands;                                 \
    }                                                          \
                                                               \
    uint8_t getCommandCount() override                         \
    {                                                          \
        return sizeof(moduleCommands) / sizeof(ModuleCommand); \
    }