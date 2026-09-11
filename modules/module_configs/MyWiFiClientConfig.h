#pragma once
#include <optional>
#include "../common/WiFiCommon.h"

struct WiFiModuleConfig {
    /// @brief WiFi SSID, max 32 chars
    const char* sta_ssid = nullptr;
    /// @brief WiFi password, max 63 chars
    const char* sta_pass = nullptr;

    /// @brief Access Point SSID, max 32 chars
    const char* ap_ssid = nullptr;
    /// @brief Access point password, max 63 chars
    const char* ap_pass = nullptr;

    /// @brief WiFi mode, STA / AP / STA_AP / OFF
    std::optional<WiFiMode> mode;

    /// @brief Whether to commence WiFi connection at startup
    std::optional<bool> commence_at_startup;
    /// @brief Whether to automatically reconnect to WiFi when disconnected
    std::optional<bool> auto_reconnect;
    /// @brief The delay before reconnection attempts
    std::optional<uint32_t> reconnect_interval_ms;
    /// @brief Max reconnection attempts before giving up
    std::optional<uint8_t> max_reconnect_attempts;
};
