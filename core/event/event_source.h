#pragma once

enum EventSource
{
    SRC_APP,
    SRC_LOGGER,
    SRC_TCP,
    SRC_WIFI,
    SRC_SENSOR_TEMP
};

inline const char *toString(EventSource s)
{
    switch (s)
    {
    case SRC_LOGGER:
        return "LOGGER";

    case SRC_TCP:
        return "TCP";

    case SRC_WIFI:
        return "WIFI";

    case SRC_SENSOR_TEMP:
        return "SENSOR_TEMP";

    default:
        return "UNKNOWN";
    }
}