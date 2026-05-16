#pragma once
#include "imodule.h"

#define MAX_MODULES 16

struct ModuleEntry
{
    IModule *module;
    uint32_t lastUpdate;
};

class ModuleRegistry
{
private:
    ModuleEntry modules[MAX_MODULES];
    int count = 0;

public:
    bool add(IModule *m)
    {
        if (count >= MAX_MODULES)
            return false;
        if (!m->init())
            return false;

        modules[count].module = m;
        modules[count].lastUpdate = 0;
        count++;

        return true;
    }

    int size() { return count; }
    IModule *get(int i) { return modules[i].module; }
    ModuleEntry &getEntry(int i) { return modules[i]; }
};