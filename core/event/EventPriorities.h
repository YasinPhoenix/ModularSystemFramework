#pragma once

// You can add more if you like to make it more specific
enum EventPriority {
    PRIORITY_HIGH = 0,
    PRIORITY_NORMAL,
    PRIORITY_LOW
};

// Max amount of events dispatched in a single loop
constexpr uint8_t MAX_HIGH_TIER = 16;
constexpr uint8_t MAX_NORMAL_TIER = 8;
constexpr uint8_t MAX_LOW_TIER = 4;
