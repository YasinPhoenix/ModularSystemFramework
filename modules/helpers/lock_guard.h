#pragma once
#include <freertos/semphr.h>

// Usage: LockGuard lock(mutex);

class LockGuard
{
public:
    LockGuard(SemaphoreHandle_t m)
        : mutex(m)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
    }

    ~LockGuard()
    {
        xSemaphoreGive(mutex);
    }

private:
    SemaphoreHandle_t mutex;
};