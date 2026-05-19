#pragma once
#include <WiFi.h>

class WiFiLogger
{
public:
    void begin(const char *host, uint16_t port)
    {
        this->host = host;
        this->port = port;
    }

    void update()
    {
        if (!client.connected())
            client.connect(host, port);
    }

    void log(const char *msg)
    {
        if (client.connected())
            client.println(msg);
    }

private:
    const char *host;
    uint16_t port;
    WiFiClient client;
};