#pragma once

#include <opencv2/core.hpp>

#include "aimbot/cursor/PID.h"

struct CaptureConfig {
    int offset_x = 0;
    int offset_y = 0;
};

struct LockConfig {
    float match_radius = 60.0f;
    int lost_frames_to_unlock = 5;
};

struct HsvConfig {
    // Inclusive lower/upper HSV threshold (H, S, V).
    cv::Scalar lower{ 82, 199, 118 };
    cv::Scalar upper{ 97, 255, 255 };

    // Reject detections with area smaller than this value (in pixels).
    // Used to filter out small noise components after thresholding.
    int minArea = 1600;

    // Morphological opening kernel size (erode -> dilate) for removing small speckles.
    // 0 means disabled.
    int morphOpen = 0;
    // Morphological closing kernel size (dilate -> erode) for filling small holes.
    int morphClose = 0;
};

// Configuration for the cursor assist controller.
struct CursorAssistConfig {
    // Control loop frequency (ticks per second).
    int tick_hz = 240;

    // Deadzone radius (pixels): if |error| is small, do not move to reduce jitter.
    int deadzone_px = 2;

    // Auto-click feature:
    // If enabled, click when the cursor stays within click_dist_px for dwell_ms,
    // then enforce cooldown_ms before the next click.
    bool auto_click = false;
    double click_dist_px = 18.0;
    int dwell_ms = 120;
    int cooldown_ms = 400;

    // Minimum sleep (microseconds) in the worker loop to avoid busy-waiting.
    int min_sleep_us = 500;

    // Default PID gains and limits for X/Y axes.
    PID2D pid{
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85},
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85}
    };
};

// Style configuration for overlay rendering.
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

struct AppConfig {
    CaptureConfig capture;
    HsvConfig hsv;
    LockConfig lock;
	CursorAssistConfig cursor;
    OverlayStyle overlay;
};
