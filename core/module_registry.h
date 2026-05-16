#pragma once
#include "imodule.h"

#define MAX_MODULES 16

class ModuleRegistry {
private:
    IModule* modules[MAX_MODULES];
    int count = 0;

public:
    bool add(IModule* m) {
        if (count >= MAX_MODULES) return false;
        if (!m->init()) return false;

        modules[count++] = m;
        return true;
    }

    int size() { return count; }
    IModule* get(int i) { return modules[i]; }
};