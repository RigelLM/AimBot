#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

#include "aimbot/app/AppConfig.h"

// helper: 读取 [h,s,v] -> cv::Scalar
inline cv::Scalar Scalar3(const nlohmann::json& a, const cv::Scalar& def) {
    if (!a.is_array() || a.size() < 3) return def;
    return cv::Scalar(a[0].get<double>(), a[1].get<double>(), a[2].get<double>());
}

inline cv::Point Point2(const nlohmann::json& a, const cv::Point& def) {
    if (!a.is_array() || a.size() < 2) return def;
    return cv::Point(a[0].get<int>(), a[1].get<int>());
}

inline void from_json(const nlohmann::json& j, CaptureConfig& c) {
    c.offset_x = j.value("offset_x", c.offset_x);
    c.offset_y = j.value("offset_y", c.offset_y);
}

inline void from_json(const nlohmann::json& j, LockConfig& c) {
    c.match_radius = j.value("match_radius", c.match_radius);
    c.lost_frames_to_unlock = j.value("lost_frames_to_unlock", c.lost_frames_to_unlock);
}

inline void from_json(const nlohmann::json& j, HsvConfig& c) {
    if (j.contains("lower")) c.lower = Scalar3(j["lower"], c.lower);
    if (j.contains("upper")) c.upper = Scalar3(j["upper"], c.upper);
    c.minArea = j.value("minArea", c.minArea);
    c.morphOpen = j.value("morphOpen", c.morphOpen);
    c.morphClose = j.value("morphClose", c.morphClose);
}

inline void from_json(const nlohmann::json& j, PIDAxis& p) {
    p.kp = j.value("kp", p.kp);
    p.ki = j.value("ki", p.ki);
    p.kd = j.value("kd", p.kd);

    p.outMin = j.value("outMin", p.outMin);
    p.outMax = j.value("outMax", p.outMax);
    p.iMin = j.value("iMin", p.iMin);
    p.iMax = j.value("iMax", p.iMax);
    p.dAlpha = j.value("dAlpha", p.dAlpha);
}

inline void from_json(const nlohmann::json& j, CursorAssistConfig& c) {
    c.tick_hz = j.value("tick_hz", c.tick_hz);
    c.deadzone_px = j.value("deadzone_px", c.deadzone_px);

    c.auto_click = j.value("auto_click", c.auto_click);
    c.click_dist_px = j.value("click_dist_px", c.click_dist_px);
    c.dwell_ms = j.value("dwell_ms", c.dwell_ms);
    c.cooldown_ms = j.value("cooldown_ms", c.cooldown_ms);

    c.min_sleep_us = j.value("min_sleep_us", c.min_sleep_us);

    if (j.contains("pid")) {
        const auto& jp = j["pid"];
        if (jp.contains("x")) c.pid.x = jp["x"].get<PIDAxis>();
        if (jp.contains("y")) c.pid.y = jp["y"].get<PIDAxis>();
    }
}

inline void from_json(const nlohmann::json& j, OverlayStyle& s) {
    s.boxThickness = j.value("boxThickness", s.boxThickness);
    s.centerRadius = j.value("centerRadius", s.centerRadius);
    s.centerThickness = j.value("centerThickness", s.centerThickness);

    s.fontScale = j.value("fontScale", s.fontScale);
    s.fontThickness = j.value("fontThickness", s.fontThickness);

    if (j.contains("latencyPos")) s.latencyPos = Point2(j["latencyPos"], s.latencyPos);
    if (j.contains("fpsPos"))     s.fpsPos = Point2(j["fpsPos"], s.fpsPos);

    s.showConfidence = j.value("showConfidence", s.showConfidence);
    s.showArea = j.value("showArea", s.showArea);
    s.showIndex = j.value("showIndex", s.showIndex);
}

inline void from_json(const nlohmann::json& j, AppConfig& c) {
    if (j.contains("capture")) c.capture = j["capture"].get<CaptureConfig>();
    if (j.contains("hsv"))     c.hsv = j["hsv"].get<HsvConfig>();
    if (j.contains("lock"))    c.lock = j["lock"].get<LockConfig>();
	if (j.contains("cursor"))  c.cursor = j["cursor"].get<CursorAssistConfig>();
	if (j.contains("overlay")) c.overlay = j["overlay"].get<OverlayStyle>();
}
