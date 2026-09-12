#pragma once
#include <optional>

struct TCPModuleConfig {
    /// @brief The address to the tcp server
    const char *server_address = nullptr;
    /// @brief The port to the tcp server
    std::optional<uint16_t> server_port;

    /// @brief The name of the device to be used for identification on the tcp server
    const char *device_name = nullptr;

    /// @brief The keep-alive timeout in milliseconds.
    std::optional<uint16_t> keep_alive;
    /// @brief The connection timeout.
    std::optional<uint16_t> connection_timeout;
    /// @brief Whether to automatically connect to the TCP server.
    std::optional<bool> auto_connect;
};
