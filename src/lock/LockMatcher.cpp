#include <limits>

#include "aimbot/lock/LockMatcher.h"

namespace aimbot::lock {

    int LockMatcher::matchIndex(const Detection& locked,
        const std::vector<Detection>& dets) const {
        if (dets.empty()) return -1;

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
