#pragma once
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

inline bool LoadJsonFile(const std::string& path, nlohmann::json& out, std::string& err) {
    std::ifstream f(path);
    if (!f.is_open()) {
        err = "cannot open file: " + path;
        return false;
    }
    try {
        f >> out;
        return true;
    }
    catch (const std::exception& e) {
        err = std::string("parse error: ") + e.what();
        return false;
    }
}
