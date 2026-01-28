#include <limits>

#include "aimbot/lock/NearestToAimSelector.h"

namespace aimbot::lock
{
    int NearestToAimSelector::selectIndex(const std::vector<Detection>& dets,
        const TargetContext& ctx) const {
        if (dets.empty()) return -1;

        int bestIdx = -1;
        float bestD2 = std::numeric_limits<float>::infinity();

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