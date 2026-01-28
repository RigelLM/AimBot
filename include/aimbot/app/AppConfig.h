// app/AppConfig.h
// Central configuration definitions for the application.
//
// This header collects all tunable parameters used across the pipeline:
//   - CaptureConfig:      coordinate offsets for ROI capture vs full-screen capture
//   - LockConfig:         target lock association + unlock thresholds
//   - HsvConfig:          HSV thresholding and mask cleanup parameters
//   - CursorAssistConfig: PID + dwell-click behavior for cursor control
//   - OverlayStyle:       visualization styling and overlay toggles
//   - AppConfig:          top-level container holding all sub-configs
//
// Design notes:
// - Each config struct is default-initialized with reasonable values, so the app can
//   run without any external JSON file.
// - AppConfigJson.h provides JSON deserialization (`from_json`) to override these defaults.
#pragma once

#include <opencv2/core.hpp>

#include "aimbot/cursor/PID.h"

// ------------------------------ CaptureConfig ------------------------------
// Coordinate mapping configuration.
//
// When capturing a region-of-interest (ROI) instead of the full desktop,
// detections are produced in ROI image coordinates. To convert ROI coordinates to
// full screen coordinates:
//   screen_x = roi_x + offset_x
//   screen_y = roi_y + offset_y
//
// For full-screen capture, offsets are typically 0.
struct CaptureConfig {
    int offset_x = 0;
    int offset_y = 0;
};

// ------------------------------ LockConfig ------------------------------
// Target lock policy configuration.
//
// Used by the "lock and track" logic that tries to keep following the same target
// across frames rather than switching immediately.
struct LockConfig {
    // Maximum center distance (pixels) to associate a detection with the currently locked target.
    float match_radius = 60.0f;
    // How many consecutive frames the locked target may be missing before unlocking.
    int lost_frames_to_unlock = 5;
};

// ------------------------------ HsvConfig ------------------------------
// HSV segmentation configuration used by HsvMasker (and often by ContourDetector).
struct HsvConfig {
    // Inclusive lower/upper HSV threshold (H, S, V).
    // OpenCV HSV ranges for 8-bit images: H in [0,179], S,V in [0,255].
    cv::Scalar lower{ 82, 199, 118 };
    cv::Scalar upper{ 97, 255, 255 };

    // Reject detections with area smaller than this value (pixels).
    // Used to filter out small noise components after thresholding.
    int minArea = 1600;

    // Morphological opening kernel size (erode -> dilate) to remove small speckles.
    // 0 means disabled.
    int morphOpen = 0;

    // Morphological closing kernel size (dilate -> erode) to fill small holes.
    // 0 means disabled.
    int morphClose = 0;
};

// ------------------------------ CursorAssistConfig ------------------------------
// Configuration for the cursor assist controller (PID tracking + optional dwell click).
struct CursorAssistConfig {
    // Control loop frequency (ticks per second).
    int tick_hz = 240;

    // Deadzone radius (pixels): if |error| is small, do not move to reduce jitter.
    int deadzone_px = 2;

    // Auto-click (dwell click) feature:
    // If enabled:
    //   - when cursor is within click_dist_px of the target, start a dwell timer
    //   - if it remains in range for dwell_ms, issue a left click
    //   - enforce cooldown_ms before allowing another click
    bool auto_click = false;
    double click_dist_px = 18.0;
    int dwell_ms = 120;
    int cooldown_ms = 400;

    // Minimum sleep (microseconds) in the worker loop to avoid busy-waiting.
    // The controller will sleep at least this long each tick.
    int min_sleep_us = 500;

    // Default PID gains and limits for X/Y axes.
    // PIDAxis holds both tunable parameters (kp/ki/kd, limits, filters) and internal state,
    // but when used in config we treat it as a parameter container (state is typically reset
    // when applying a new config).
    PID2D pid{
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85},
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85}
    };
};

// ------------------------------ OverlayStyle ------------------------------
// Style configuration for overlay rendering (debug visualization).
// All values are in pixel units unless otherwise noted.
struct OverlayStyle {
    // Bounding box line thickness.
    int boxThickness = 2;

    // Radius of the center marker (circle) drawn at Detection.center.
    int centerRadius = 6;

    // Thickness of the center marker.
    // -1 means a filled circle in OpenCV.
    int centerThickness = -1; // -1 = filled

    // Text rendering parameters for cv::putText.
    double fontScale = 0.6;
    int fontThickness = 2;

    // On-screen positions for performance text overlays.
    cv::Point latencyPos{ 10, 30 };
    cv::Point fpsPos{ 10, 60 };

    // Label content toggles.
    bool showConfidence = true;
    bool showArea = false;
    bool showIndex = false;
};

// ------------------------------ AppConfig ------------------------------
// Top-level configuration grouping all sub-systems.
// This is typically loaded from JSON (AppConfigJson.h) and then passed around
// to initialize the capture, vision, lock logic, cursor assist, and overlay modules.
struct AppConfig {
    CaptureConfig capture;
    HsvConfig hsv;
    LockConfig lock;
	CursorAssistConfig cursor;
    OverlayStyle overlay;
};
