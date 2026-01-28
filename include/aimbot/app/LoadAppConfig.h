// app/LoadAppConfig.h
// Convenience helper to load application configuration from a JSON file,
// falling back to default configuration on any error.
//
// Flow:
//  1) Start with default-constructed AppConfig (defaults defined in AppConfig).
//  2) Read JSON from disk via LoadJsonFile().
//     - If the file cannot be opened or JSON parsing fails, log the error and return defaults.
//  3) Convert JSON -> AppConfig via from_json(j, cfg) (defined in AppConfigJson.h).
//     - If conversion/validation throws, log the error and return defaults.
//  4) On success, log that the config was loaded and return the filled AppConfig.
//
// Notes:
// - Uses stderr for logging so it doesn't interfere with normal program output.
// - This is header-only (inline) to keep wiring simple for small applications.
// - If you want stricter behavior (fail-fast), you can create a separate loader that throws.
#pragma once

#include <iostream>
#include <string>

#include "aimbot/config/JsonLoader.h"
#include "aimbot/app/AppConfig.h"
#include "aimbot/app/AppConfigJson.h"

// Load AppConfig from a JSON file.
// If the file is missing, invalid JSON, or cannot be applied to AppConfig,
// this function prints a message and returns the default config instead.
//
// Params:
//   path : path to the JSON config file
//
// Returns:
//   AppConfig loaded from file on success, otherwise default AppConfig.
inline AppConfig LoadAppConfigOrDefault(const std::string& path) {
    // Default config (used as fallback).
    AppConfig cfg;

    // 1) Read JSON file.
    nlohmann::json j;
    std::string err;
    if (!LoadJsonFile(path, j, err)) {
        std::cerr << "[config] " << err << " (using defaults)\n";
        return cfg;
    }

    // 2) Apply JSON to AppConfig.
    try {
        // from_json(...) is expected to be provided by AppConfigJson.h
       // (nlohmann::json conversion hook).
        from_json(j, cfg);
    }
    catch (const std::exception& e) {
        std::cerr << "[config] apply error: " << e.what() << " (using defaults)\n";
        return AppConfig{};
    }

    // 3) Success.
    std::cerr << "[config] loaded: " << path << "\n";
    return cfg;
}
