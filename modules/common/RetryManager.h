#pragma once
#include "LockGuard.h"
#include <functional>

class RetryManager {
public:
    using Action = std::function<bool()>;
    using ExhaustedCallback = std::function<void()>;
    using FailedAttemptCallback = std::function<void(uint8_t attemptNumber)>;

    RetryManager() {
        mutex = xSemaphoreCreateMutex();
    }

    ~RetryManager() {
        vSemaphoreDelete(mutex);
    }

    // Not copyable/movable — owns a raw SemaphoreHandle_t
    RetryManager(const RetryManager &) = delete;
    RetryManager &operator=(const RetryManager &) = delete;

    void init(uint8_t maxAttempts, uint32_t intervalMs, Action action) {
        LockGuard lock(mutex);
        this->maxAttempts = maxAttempts;
        this->intervalMs = intervalMs;
        this->action = std::move(action);
    }

    void update() {
        bool shouldFireExhausted = false;
        bool shouldCallAction = false;

        {
            LockGuard lock(mutex);

            if (!armed)
                return;

            if (attempts >= maxAttempts) {
                armed = false;
                shouldFireExhausted = true;
            } else {
                uint32_t now = millis();
                if (now - lastAttemptMs >= intervalMs) {
                    lastAttemptMs = now;
                    attempts++;
                    shouldCallAction = true;
                }
            }
        } // lock released here

        // Callbacks run outside the lock — safe for them to call
        // arm()/disarm()/reportSuccess() without deadlocking.
        if (shouldFireExhausted) {
            if (onExhausted)
                onExhausted();
            return;
        }

        if (shouldCallAction) {
            bool started = action && action();
            if (!started && onAttemptFailed) {
                uint8_t attemptSnapshot;
                {
                    LockGuard lock(mutex);
                    attemptSnapshot = attempts;
                }
                onAttemptFailed(attemptSnapshot);
            }
        }
    }

    void arm() {
        LockGuard lock(mutex);
        armed = true;
        attempts = 0;
        lastAttemptMs = 0;
    }

    void disarm() {
        LockGuard lock(mutex);
        armed = false;
    }

    void reportSuccess() {
        LockGuard lock(mutex);
        armed = false;
        attempts = 0;
    }

    void onExhaustedDo(ExhaustedCallback cb) { onExhausted = std::move(cb); }
    void onAttemptFailedDo(FailedAttemptCallback cb) { onAttemptFailed = std::move(cb); }

    bool isArmed() {
        LockGuard lock(mutex);
        return armed;
    }

    uint8_t attemptCount() {
        LockGuard lock(mutex);
        return attempts;
    }

private:
    uint8_t maxAttempts;
    uint32_t intervalMs;
    Action action;
    ExhaustedCallback onExhausted;
    FailedAttemptCallback onAttemptFailed;

    SemaphoreHandle_t mutex;
    bool armed = false;
    uint8_t attempts = 0;
    uint32_t lastAttemptMs = 0;
};