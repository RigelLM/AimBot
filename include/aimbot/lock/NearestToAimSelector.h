#pragma once
#include "ITargetSelector.h"

namespace aimbot::lock
{
    // Default selector: choose detection whose center is nearest to ctx.aimPointImg.
    class NearestToAimSelector final : public ITargetSelector {
    public:
        int selectIndex(const std::vector<Detection>& dets,
            const TargetContext& ctx) const override;
    };
}