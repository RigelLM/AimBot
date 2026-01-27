#pragma once
#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

#include "aimbot/app/AppConfig.h"

// helper: 读取 [h,s,v] -> cv::Scalar
inline cv::Scalar Scalar3(const nlohmann::json& a, const cv::Scalar& def) {
    if (!a.is_array() || a.size() < 3) return def;
    return cv::Scalar(a[0].get<double>(), a[1].get<double>(), a[2].get<double>());
}

inline void from_json(const nlohmann::json& j, CaptureConfig& c) {
    c.offset_x = j.value("offset_x", c.offset_x);
    c.offset_y = j.value("offset_y", c.offset_y);
}

inline void from_json(const nlohmann::json& j, LockConfig& c) {
    c.match_radius = j.value("match_radius", c.match_radius);
    c.lost_frames_to_unlock = j.value("lost_frames_to_unlock", c.lost_frames_to_unlock);
}

// 你已有 HsvConfig：这里给它加 from_json（覆盖默认值）
inline void from_json(const nlohmann::json& j, HsvConfig& c) {
    if (j.contains("lower")) c.lower = Scalar3(j["lower"], c.lower);
    if (j.contains("upper")) c.upper = Scalar3(j["upper"], c.upper);
    c.minArea = j.value("minArea", c.minArea);
    c.morphOpen = j.value("morphOpen", c.morphOpen);
    c.morphClose = j.value("morphClose", c.morphClose);
}

inline void from_json(const nlohmann::json& j, AppConfig& c) {
    if (j.contains("capture")) c.capture = j["capture"].get<CaptureConfig>();
    if (j.contains("hsv"))     c.hsv = j["hsv"].get<HsvConfig>();
    if (j.contains("lock"))    c.lock = j["lock"].get<LockConfig>();
}
