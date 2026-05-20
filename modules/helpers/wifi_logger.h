#pragma once
#include <WiFi.h>

class WiFiLogger
{
public:
    void begin(const char *host, uint16_t port)
    {
        this->host = host;
        this->port = port;
        isBegun = true;
    }

    void update()
    {
        if (!client.connected() && isBegun)
            client.connect(host, port);
    }

    void log(const char *msg)
    {
        if (client.connected() && isBegun)
            client.println(msg);
    }

private:
    bool isBegun = false;

    const char *host;
    uint16_t port;
    WiFiClient client;
};