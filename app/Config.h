#pragma once
#include "../modules/module_configs/ModuleConfigs.h"

WiFiModuleConfig AppWiFiConfig = {
    .sta_ssid = "OMEGA",
    .sta_pass = "65",
    .mode = WiFiMode::STA,
    .commence_at_startup = true,
    .max_reconnect_attempts = 5,
};
