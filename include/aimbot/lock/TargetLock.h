// lock/TargetLock.h
// Target lock state machine.
//
// Purpose:
// - Keep tracking the same target across frames ("lock") to avoid jittery target switching.
// - When the locked target cannot be matched in the current frame, do NOT immediately switch.
//   Instead, count consecutive misses and only unlock after cfg_.lost_frames_to_unlock frames.
// - When unlocked, delegate target selection to an injected policy object (ITargetSelector).
//
// Key ideas:
// - Separation of concerns:
//     * TargetLock = lock/unlock state machine + bookkeeping
//     * ITargetSelector = policy for choosing a new target when unlocked
//     * LockMatcher = association logic to match current detections to the previously locked target
// - Uses std::optional to represent "no lock" or "no target this frame".
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
        // Construct with lock config and a target selection strategy.
        //
        // cfg      : lock association radius and unlock thresholds
        // selector : policy used to pick a new target when unlocked
        TargetLock(::LockConfig cfg, std::unique_ptr<ITargetSelector> selector);

        // Update lock state using detections from the current frame.
        //
        // dets : list of detections in current frame (image coordinates)
        // ctx  : extra context for selection (e.g., cursor position, ROI offsets, etc.)
        //
        // Returns:
        //   - index of the detection to track THIS frame (if matched or newly selected)
        //   - std::nullopt if there is no valid target this frame
        //     (e.g., locked target is temporarily missing but not yet unlocked)
        std::optional<int> update(const std::vector<Detection>& dets,
            const TargetContext& ctx);

        // Reset lock state: clear current lock and miss counters.
        void reset();

        // Whether a target is currently locked (even if it might be temporarily lost this frame).
        bool isLocked() const { return locked_.has_value(); }
        // Number of consecutive frames the locked target has not been matched.
        int lostFrames() const { return lostFrames_; }

        // Last known locked center in image coordinates.
        // This remains meaningful even when the target is temporarily lost (before unlocking).
        cv::Point2f lockedCenterImg() const { return locked_ ? locked_->center : cv::Point2f(0.f, 0.f); }

    private:
        // Lock parameters.
        ::LockConfig cfg_;

        // Selection strategy used only when unlocked.
        std::unique_ptr<ITargetSelector> selector_;

        // Matching logic to associate current detections with previous lock.
        LockMatcher matcher_;

        // Currently locked detection (may represent last known target state).
        std::optional<Detection> locked_;
        
        // Consecutive miss counter while locked.
        int lostFrames_ = 0;
    };

} // namespace aimbot::lock
