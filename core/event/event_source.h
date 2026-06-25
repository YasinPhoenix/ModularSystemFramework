#pragma once

enum EventSource
{
    SRC_APP,
    SRC_SYSTEM,
    SRC_SERIAL,
    SRC_TCP,
    SRC_WIFI
};

inline const char *toString(EventSource s)
{
    switch (s)
    {
    case SRC_APP:
        return "APP";

    case SRC_SYSTEM:
        return "System";

    case SRC_SERIAL:
        return "SERIAL";

    case SRC_TCP:
        return "TCP";

    case SRC_WIFI:
        return "WIFI";

    default:
        return "UNKNOWN";
    }
}