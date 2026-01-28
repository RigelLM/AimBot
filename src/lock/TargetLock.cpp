#include "aimbot/lock/TargetLock.h"
#include "aimbot/lock/NearestToAimSelector.h"

namespace aimbot::lock {

    TargetLock::TargetLock(::LockConfig cfg, std::unique_ptr<ITargetSelector> selector)
        : cfg_(cfg), selector_(std::move(selector)), matcher_(cfg_) {
        // If caller passes nullptr, fall back to the default selector.
        if (!selector_) {
            selector_ = std::make_unique<NearestToAimSelector>();
        }
    }

    std::optional<int> TargetLock::update(const std::vector<Detection>& dets,
        const TargetContext& ctx) {
        int chosen = -1;

        // 1) If locked: attempt to re-associate with the same target.
        if (locked_) {
            chosen = matcher_.matchIndex(*locked_, dets);

            if (chosen >= 0) {
                locked_ = dets[chosen];
                lostFrames_ = 0;
                return chosen;
            }

            // Not found this frame: keep lock (sticky), but count misses.
            lostFrames_++;

            if (lostFrames_ >= cfg_.lost_frames_to_unlock) {
                // Consider truly gone.
                locked_.reset();
                lostFrames_ = 0;
            }
            else {
                // Temporarily lost: do not switch yet.
                return std::nullopt;
            }
        }

        // 2) If unlocked: select a new target.
        if (!locked_) {
            chosen = selector_->selectIndex(dets, ctx);
            if (chosen >= 0) {
                locked_ = dets[chosen];
                lostFrames_ = 0;
                return chosen;
            }
        }

        return std::nullopt;
    }

    void TargetLock::reset() {
        locked_.reset();
        lostFrames_ = 0;
    }

} // namespace aimbot::lock
