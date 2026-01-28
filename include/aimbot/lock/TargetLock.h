#pragma once

#include <optional>
#include <memory>
#include <vector>

#include <opencv2/core.hpp>

#include "aimbot/vision/Detection.h"
#include "aimbot/app/AppConfig.h"
#include "aimbot/lock/TargetContext.h"
#include "aimbot/lock/ITargetSelector.h"
#include "aimbot/lock/LockMatcher.h"

namespace aimbot::lock {

    // Target lock state machine.
    // - Maintains a lock across frames
    // - Unlocks only after cfg.lost_frames_to_unlock consecutive misses
    // - When unlocked, uses ITargetSelector to choose a new target
    class TargetLock {
    public:
        TargetLock(::LockConfig cfg, std::unique_ptr<ITargetSelector> selector);

        // Update lock state with current detections.
        // Returns:
        //   - index of the detection to track THIS frame if found/selected
        //   - std::nullopt if there is no valid target this frame (e.g., temporarily lost)
        std::optional<int> update(const std::vector<Detection>& dets,
            const TargetContext& ctx);

        void reset();

        bool isLocked() const { return locked_.has_value(); }
        int lostFrames() const { return lostFrames_; }

        // Last known locked center (image coordinates). Meaningful even when temporarily lost.
        cv::Point2f lockedCenterImg() const { return locked_ ? locked_->center : cv::Point2f(0.f, 0.f); }

    private:
        ::LockConfig cfg_;
        std::unique_ptr<ITargetSelector> selector_;
        LockMatcher matcher_;

        std::optional<Detection> locked_;
        int lostFrames_ = 0;
    };

} // namespace aimbot::lock
