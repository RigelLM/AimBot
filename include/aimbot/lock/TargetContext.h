#pragma once

#include <opencv2/core.hpp>

namespace aimbot::lock {

    // Per-frame context for selection/scoring.
    // All coordinates here are in *image coordinates* (i.e., capture ROI space).
    struct TargetContext {
        cv::Point2f aimPointImg{ 0.f, 0.f }; // e.g., cursor position mapped into image coords, or screen-center in image coords
        float dt = 0.0f;                  // seconds (optional for future predictors)
    };

}