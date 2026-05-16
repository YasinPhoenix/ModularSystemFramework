#pragma once
#include "event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define EVENT_QUEUE_SIZE 32

class EventQueue
{
private:
    Event buffer[EVENT_QUEUE_SIZE];
    int head = 0;
    int tail = 0;

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

public:
    bool push(const Event &e)
    {
        portENTER_CRITICAL(&mux);

        int next = (head + 1) % EVENT_QUEUE_SIZE;
        if (next == tail)
        {
            portEXIT_CRITICAL(&mux);
            return false;
        }

        buffer[head] = e;
        head = next;

        portEXIT_CRITICAL(&mux);
        return true;
    }

    bool pop(Event &out)
    {
        portENTER_CRITICAL(&mux);

        if (tail == head)
        {
            portEXIT_CRITICAL(&mux);
            return false;
        }

        out = buffer[tail];
        tail = (tail + 1) % EVENT_QUEUE_SIZE;

        portEXIT_CRITICAL(&mux);
        return true;
    }

    const Event &getBufferIndex(size_t index) { return buffer[index]; }
};