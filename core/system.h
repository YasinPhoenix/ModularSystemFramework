#pragma once
#include "module_registry.h"
#include "event_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class System
{
private:
    ModuleRegistry registry;
    EventQueue highQueue;
    EventQueue normalQueue;
    EventQueue lowQueue;

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
        return registry.add(m);
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