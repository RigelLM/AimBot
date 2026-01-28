// lock/LockMatcher.h
// Association logic for maintaining a target lock across frames.
//
// Responsibility:
// - Given the previously locked target (from last frame / last known state)
//   and the list of current-frame detections, decide whether any detection
//   corresponds to the same target.
// - If a match exists, return the index of the best candidate; otherwise return -1.
//
// Typical matching heuristic (implemented in .cpp):
// - Compare detection centers to the locked center.
// - Choose the closest detection within cfg_.match_radius.
// - If none are within the radius, consider the target "missing" for this frame.
//
// Notes:
// - This class does NOT decide when to unlock permanently; that is handled by TargetLock
//   via cfg_.lost_frames_to_unlock and a consecutive miss counter.
#pragma once

#include <vector>

#include "aimbot/vision/Detection.h"
#include "aimbot/app/AppConfig.h"

namespace aimbot::lock
{
    // Associates current-frame detections with the previously locked target.
    class LockMatcher {
    public:
        // Construct with lock parameters (association radius, etc.).
        explicit LockMatcher(::LockConfig cfg) : cfg_(cfg) {}

        // Match the currently locked target to one of the current-frame detections.
       //
       // locked : last known locked detection (image coordinates)
       // dets   : current-frame detections (image coordinates)
       //
       // Returns:
       //   - index of the best matching detection in dets
       //   - -1 if no detection is close enough to be considered the same target
        int matchIndex(const Detection& locked,
            const std::vector<Detection>& dets) const;

    private:
        // Matching configuration (e.g., match_radius).
        ::LockConfig cfg_;
    };
}