// config/JsonLoader.h
// Small helper for loading a JSON file into a nlohmann::json object.
//
// Behavior:
// - Opens the file at `path` using std::ifstream
// - Parses JSON via nlohmann::json stream operator (f >> out)
// - Returns true on success
// - On failure, returns false and writes a human-readable message to `err`
//
// Notes:
// - This function does not validate schema; it only parses JSON syntax.
// - The file is opened in text mode (default for std::ifstream).
// - If you want UTF-8 paths on Windows, prefer passing a UTF-8 encoded std::string
//   and ensure your toolchain uses /utf-8, or use std::filesystem::path.
#pragma once

#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

// Load a JSON file from disk.
//
// Params:
//   path : file path to read
//   out  : parsed JSON output
//   err  : error message output (only meaningful when return == false)
//
// Returns:
//   true  if the file was opened and parsed successfully
//   false if the file could not be opened or parsing failed
inline bool LoadJsonFile(const std::string& path, nlohmann::json& out, std::string& err) {
    std::ifstream f(path);
    if (!f.is_open()) {
        err = "cannot open file: " + path;
        return false;
    }
    try {
        // Parse JSON from stream.
        f >> out;
        return true;
    }
    catch (const std::exception& e) {
        // Propagate parsing error message.
        err = std::string("parse error: ") + e.what();
        return false;
    }
}
