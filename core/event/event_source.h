#pragma once

enum EventSource
{
    SRC_LOGGER,
    SRC_WIFI,
    SRC_SENSOR_TEMP
};

inline const char *toString(EventSource s)
{
    switch (s)
    {
    case SRC_LOGGER:
        return "SRC_LOGGER";

    case SRC_WIFI:
        return "SRC_WIFI";

    case SRC_SENSOR_TEMP:
        return "SRC_SENSOR_TEMP";

    default:
        return "SRC_UNKNOWN";
    }
}