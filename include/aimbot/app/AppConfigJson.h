// app/AppConfigJson.h
// JSON deserialization helpers for application configuration.
//
// This header defines `from_json(...)` overloads used by nlohmann::json to convert
// JSON objects into the project's config structs (AppConfig and its sub-configs).
//
// Design goals:
// - Keep config loading lightweight and header-only (inline).
// - Provide sensible defaults: missing keys keep the current struct values.
// - Parse common OpenCV types from JSON arrays (cv::Scalar, cv::Point).
//
// Expected JSON shape (example):
// {
//   "capture": { "offset_x": 0, "offset_y": 0 },
//   "hsv": {
//     "lower": [82,199,118],
//     "upper": [97,255,255],
//     "minArea": 1600,
//     "morphOpen": 0,
//     "morphClose": 0
//   },
//   "lock": { "match_radius": 60.0, "lost_frames_to_unlock": 5 },
//   "cursor": {
//     "tick_hz": 240,
//     "deadzone_px": 2,
//     "auto_click": true,
//     "click_dist_px": 16.0,
//     "dwell_ms": 150,
//     "cooldown_ms": 150,
//     "min_sleep_us": 500,
//     "pid": { "x": { ... }, "y": { ... } }
//   },
//   "overlay": { "showConfidence": true, "latencyPos": [10,30] }
// }
#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

#include "aimbot/app/AppConfig.h"

// Parse a JSON array of length >= 3 into cv::Scalar(a0, a1, a2).
// If the JSON value is not a valid array, return the provided default.
inline cv::Scalar Scalar3(const nlohmann::json& a, const cv::Scalar& def) {
    if (!a.is_array() || a.size() < 3) return def;
    return cv::Scalar(a[0].get<double>(), a[1].get<double>(), a[2].get<double>());
}

// Parse a JSON array of length >= 2 into cv::Point(x, y).
// If the JSON value is not a valid array, return the provided default.
inline cv::Point Point2(const nlohmann::json& a, const cv::Point& def) {
    if (!a.is_array() || a.size() < 2) return def;
    return cv::Point(a[0].get<int>(), a[1].get<int>());
}

// ------------------------------ Sub-configs ------------------------------

// Capture settings (e.g., when capturing ROI instead of full screen).
// Missing keys keep existing defaults in `c`.
inline void from_json(const nlohmann::json& j, CaptureConfig& c) {
    c.offset_x = j.value("offset_x", c.offset_x);
    c.offset_y = j.value("offset_y", c.offset_y);
}

// Target lock settings for association + unlock logic.
// Missing keys keep existing defaults in `c`.
inline void from_json(const nlohmann::json& j, LockConfig& c) {
    c.match_radius = j.value("match_radius", c.match_radius);
    c.lost_frames_to_unlock = j.value("lost_frames_to_unlock", c.lost_frames_to_unlock);
}

// HSV segmentation settings.
// lower/upper are parsed from arrays: [H, S, V].
// Other fields use j.value(...) with fallback to existing defaults.
inline void from_json(const nlohmann::json& j, HsvConfig& c) {
    if (j.contains("lower")) c.lower = Scalar3(j["lower"], c.lower);
    if (j.contains("upper")) c.upper = Scalar3(j["upper"], c.upper);
    c.minArea = j.value("minArea", c.minArea);
    c.morphOpen = j.value("morphOpen", c.morphOpen);
    c.morphClose = j.value("morphClose", c.morphClose);
}

// PID axis parameters (for X or Y).
// Note: PID internal state (integral/prevError/prevDeriv) is not loaded here;
// only tunable parameters are loaded.
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

// Cursor assist controller configuration.
// Supports nested PID config under:
//   "pid": { "x": { ... }, "y": { ... } }
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

// Overlay visualization style configuration.
// Supports optional positions as arrays: "latencyPos": [x,y], "fpsPos": [x,y]
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

// ------------------------------ Root config ------------------------------

// AppConfig is composed of multiple sub-config objects.
// Each section is optional; if a key is missing, that section remains at defaults.
inline void from_json(const nlohmann::json& j, AppConfig& c) {
    if (j.contains("capture")) c.capture = j["capture"].get<CaptureConfig>();
    if (j.contains("hsv"))     c.hsv = j["hsv"].get<HsvConfig>();
    if (j.contains("lock"))    c.lock = j["lock"].get<LockConfig>();
	if (j.contains("cursor"))  c.cursor = j["cursor"].get<CursorAssistConfig>();
	if (j.contains("overlay")) c.overlay = j["overlay"].get<OverlayStyle>();
}
