// lock/NearestToAimSelector.h
// Default target selection policy.
//
// This selector implements a simple and robust heuristic:
//   - Choose the detection whose center is closest to a reference "aim point"
//     provided in TargetContext (ctx.aimPointImg).
//
// Typical aim points:
// - Cursor position mapped into image coordinates
// - Screen center mapped into image coordinates
//
// Notes:
// - This policy is only used when the system is *unlocked* (i.e., needs a new target).
// - When locked, TargetLock typically uses a separate matching strategy (LockMatcher)
//   to keep tracking the same target across frames.
#pragma once
#include "ITargetSelector.h"

namespace aimbot::lock
{
    // Default selector: choose detection whose center is nearest to ctx.aimPointImg.
    class NearestToAimSelector final : public ITargetSelector {
    public:
        // Select a target index from the current frame detections.
        //
        // dets : current detections (image coordinates)
        // ctx  : per-frame context containing the aim reference point
        //
        // Returns:
        //   - index in [0, dets.size()) for the chosen detection
        //   - -1 if dets is empty (or no valid candidate)
        int selectIndex(const std::vector<Detection>& dets,
            const TargetContext& ctx) const override;
    };
}