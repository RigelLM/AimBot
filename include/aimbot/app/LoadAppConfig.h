#pragma once

#include <iostream>
#include <string>

#include "aimbot/config/JsonLoader.h"
#include "aimbot/app/AppConfig.h"
#include "aimbot/app/AppConfigJson.h"

inline AppConfig LoadAppConfigOrDefault(const std::string& path) {
    AppConfig cfg;

    nlohmann::json j;
    std::string err;
    if (!LoadJsonFile(path, j, err)) {
        std::cerr << "[config] " << err << " (using defaults)\n";
        return cfg;
    }

    try {
        from_json(j, cfg);
    }
    catch (const std::exception& e) {
        std::cerr << "[config] apply error: " << e.what() << " (using defaults)\n";
        return AppConfig{};
    }

    std::cerr << "[config] loaded: " << path << "\n";
    return cfg;
}
