#pragma once
#include "Event.h"
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

    bool pushCoalesced(const Event &e)
    {
        portENTER_CRITICAL(&mux);

        // Only walk the ACTIVE window [tail, head)
        for (int i = tail; i != head; i = (i + 1) % EVENT_QUEUE_SIZE)
        {
            if (buffer[i].type == e.type)
            {
                buffer[i] = e; // real slot, real overwrite
                portEXIT_CRITICAL(&mux);
                return true;
            }
        }

        // No match found — push normally (queue-full check included)
        int next = (head + 1) % EVENT_QUEUE_SIZE;
        if (next == tail)
        {
            portEXIT_CRITICAL(&mux);
            return false; // queue full
        }

        buffer[head] = e;
        head = next;

        portEXIT_CRITICAL(&mux);
        return true;
    }
};