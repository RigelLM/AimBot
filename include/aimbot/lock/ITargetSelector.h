// lock/ITargetSelector.h
// Target selection strategy interface.
//
// Purpose:
// - When the system is NOT locked (no current target), we need a policy to choose
//   a new target from the list of detections.
// - Different policies can be swapped in without changing the lock state machine:
//     * nearest-to-aim point
//     * highest confidence / largest area
//     * class-based priority (if using an ML detector later)
//     * multi-factor scoring, etc.
//
// This interface is typically used by TargetLock:
// - If unlocked, TargetLock calls selector_->selectIndex(dets, ctx)
// - If locked, TargetLock uses LockMatcher to keep tracking the same target
#pragma once
#include <vector>

#include "aimbot/vision/Detection.h"
#include "aimbot/lock/TargetContext.h"

namespace aimbot::lock {
    // Strategy interface: choose a new target when not locked.
    class ITargetSelector {
    public:
        virtual ~ITargetSelector() = default;

        // Choose a detection from the current frame.
        //
        // dets : current detections (image coordinates)
        // ctx  : per-frame context (e.g., aim point, dt, etc.)
        //
        // Returns:
        //   - index in [0, dets.size()) for the chosen detection
        //   - -1 to choose nothing (e.g., no suitable candidate)
        virtual int selectIndex(const std::vector<Detection>& dets,
            const TargetContext& ctx) const = 0;
    };
}
