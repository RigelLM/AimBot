// NearestToAimSelector.cpp
// Default target selection strategy: choose the detection whose center is closest
// to a reference "aim point" provided by TargetContext.
//
// This selector is typically used when the system is *unlocked* (no current target).
// The aim point can be:
// - cursor position mapped into image coordinates (ROI space), or
// - screen center mapped into image coordinates, etc.
//
// Selection rule:
//   argmin_i || dets[i].center - ctx.aimPointImg ||^2
// We use squared distance to avoid an unnecessary sqrt().

#include <limits>

#include "aimbot/lock/NearestToAimSelector.h"

namespace aimbot::lock
{
    int NearestToAimSelector::selectIndex(const std::vector<Detection>& dets,
        const TargetContext& ctx) const {
        if (dets.empty()) return -1;

        int bestIdx = -1;
        float bestD2 = std::numeric_limits<float>::infinity();

        // Iterate all detections and pick the one with minimum squared distance to aimPointImg.
        for (int i = 0; i < static_cast<int>(dets.size()); ++i) {
            const float dx = dets[i].center.x - ctx.aimPointImg.x;
            const float dy = dets[i].center.y - ctx.aimPointImg.y;
            const float d2 = dx * dx + dy * dy;

            if (d2 < bestD2) {
                bestD2 = d2;
                bestIdx = i;
            }
        }

        return bestIdx;
    }
}