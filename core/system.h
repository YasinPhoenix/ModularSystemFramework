#pragma once
#include "module_registry.h"
#include "event_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class System
{
private:
    ModuleRegistry registry;
    EventQueue queue;

    TaskHandle_t updateTaskHandle = nullptr;

    static void updateTask(void *arg)
    {
        System *sys = static_cast<System *>(arg);

        while (true)
        {
            for (int i = 0; i < sys->registry.size(); i++)
            {
                sys->registry.get(i)->update();
            }

            vTaskDelay(1);
        }
    }

public:
    bool addModule(IModule *m)
    {
        return registry.add(m);
    }

    EventQueue &events() { return queue; }

    void emit(const Event &e)
    {
        queue.push(e);
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

        while (queue.pop(e))
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
    }
};