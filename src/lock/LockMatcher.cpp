// LockMatcher.cpp
// Association logic for maintaining a lock across frames.
//
// Given:
//   - `locked`: the previously locked target (last known detection)
//   - `dets`  : current-frame detections
//
// This matcher finds the detection that most likely corresponds to the same target.
// Current heuristic:
//   - Compute squared distance between centers
//   - Only consider detections within cfg_.match_radius
//   - Choose the closest one within that radius
//
// Returns:
//   - index of best match in dets
//   - -1 if no detection is within the association radius
//
// Notes:
// - Uses squared distances to avoid sqrt().
// - This class only performs matching; unlocking policy (miss counters) is handled by TargetLock.

#include <limits>

#include "aimbot/lock/LockMatcher.h"

namespace aimbot::lock {

    int LockMatcher::matchIndex(const Detection& locked,
        const std::vector<Detection>& dets) const {
        // No detections => no possible match.
        if (dets.empty()) return -1;

        // Compare squared distances to avoid sqrt().
        const float r2 = cfg_.match_radius * cfg_.match_radius;

        int best = -1;
        float bestD2 = std::numeric_limits<float>::infinity();

        for (int i = 0; i < static_cast<int>(dets.size()); ++i) {
            const float dx = dets[i].center.x - locked.center.x;
            const float dy = dets[i].center.y - locked.center.y;
            const float d2 = dx * dx + dy * dy;

            // Closest detection within the association radius.
            if (d2 <= r2 && d2 < bestD2) {
                bestD2 = d2;
                best = i;
            }
        }

        return best;
    }

}
