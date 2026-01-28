// lock/TargetContext.h
// Per-frame context passed into target selection / locking logic.
//
// Motivation:
// - Detection lists alone are often insufficient to choose the "best" target.
// - We typically need extra per-frame information such as where the user is aiming,
//   cursor position (mapped into image/ROI coordinates), time delta, etc.
// - This struct centralizes that information so selectors/matchers can make consistent
//   decisions without reaching into unrelated modules.
//
// Coordinate convention:
// - All fields are expressed in *image coordinates* (capture ROI space).
//   If you capture full screen, image coords == screen coords.
//   If you capture an ROI, image coords are relative to the ROI top-left.
#pragma once

#include <opencv2/core.hpp>

namespace aimbot::lock {

    // Per-frame context for selection/scoring.
    // All coordinates here are in *image coordinates* (i.e., capture ROI space).
    struct TargetContext {
        // Reference point for selection, in image coordinates.
        // Typical choices:
        // - cursor position mapped to image coords (cursor_x - offset_x, cursor_y - offset_y)
        // - screen center mapped to image coords
        // - predicted aim point from a motion model (future extension)
        cv::Point2f aimPointImg{ 0.f, 0.f };
        
        // Time delta since last frame (seconds).
        // Optional for future predictors/filters (e.g., velocity estimation, Kalman filter).
        float dt = 0.0f;
    };

}