#pragma once
#include <vector>

#include "aimbot/vision/Detection.h"
#include "aimbot/lock/TargetContext.h"

namespace aimbot::lock {
    // Strategy interface: choose a new target when not locked.
    class ITargetSelector {
    public:
        virtual ~ITargetSelector() = default;

        // Return the index of the chosen detection, or -1 to choose nothing.
        virtual int selectIndex(const std::vector<Detection>& dets,
            const TargetContext& ctx) const = 0;
    };
}
