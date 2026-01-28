// CursorAssist.cpp
// Implementation of CursorAssistController.
//
// Responsibilities:
// - Maintain a worker thread that runs a control loop at (roughly) cfg.tick_hz
// - Read the current cursor position via ICursorBackend
// - If enabled and a target exists, compute error = target - cursor
// - Apply PID (per axis) to generate bounded relative cursor movement (dx, dy)
// - Optionally perform dwell click: click after staying within a distance threshold
//   for a dwell time, with a cooldown between clicks
//
// Thread-safety:
// - cfg_ and target_ are protected by mtx_
// - enabled_ and running_ are atomics
// - condition_variable is used to wake the worker when config/target/enabled changes

#include "aimbot/cursor/CursorAssist.h"

// Clamp helper for double values.
static inline double clampd(double v, double lo, double hi) {
    return std::max(lo, std::min(v, hi));
}

CursorAssistController::CursorAssistController(ICursorBackend& backend)
    : backend_(backend) {
    // backend_ must outlive this controller
}

CursorAssistController::~CursorAssistController() {
    // Ensure worker thread is stopped before destruction.
    stop();
}

void CursorAssistController::start() {
    // Start only once (CAS on running_).
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;

    // Spawn worker thread.
    worker_ = std::thread(&CursorAssistController::threadMain, this);
}

void CursorAssistController::stop() {
    // Stop only if we were running.
    if (!running_.exchange(false)) return;

    // Wake worker so it can exit promptly.
    cv_.notify_all();

    if (worker_.joinable()) worker_.join();
}

void CursorAssistController::setEnabled(bool en) {
    // Enable/disable the control action.
    bool wasEnabled = enabled_.exchange(en, std::memory_order_relaxed);

    if (en && !wasEnabled) {
        std::lock_guard<std::mutex> lk(mtx_);

        // 1) reset PID state(prevent setpoint kick)
        cfg_.pid.reset();

        // 2) reset dwell click timer
        inRangeSince_ = {};
        lastClick_ = {};
    }

    // Wake the worker so state changes take effect immediately.
    cv_.notify_all();
}

void CursorAssistController::updateConfig(const CursorAssistConfig& cfg) {
    {
        // Replace config atomically under lock.
        std::lock_guard<std::mutex> lk(mtx_);
        cfg_ = cfg;

        // Reset PID internal state to avoid carrying over integral/derivative memory.
        // cfg_.pid.reset();
        pidState_ = cfg_.pid;
        pidState_.reset();
    }

    // Wake worker so new config is used immediately.
    cv_.notify_all();
}

CursorAssistConfig CursorAssistController::getConfigCopy() const {
    // Return a copy for thread-safe inspection.
    std::lock_guard<std::mutex> lk(mtx_);
    return cfg_;
}

void CursorAssistController::setTarget(const CursorPoint& p) {
    {
        // Set current target under lock.
        std::lock_guard<std::mutex> lk(mtx_);

        if (target_.has_value()) {
            int dx = p.x - target_->x;
            int dy = p.y - target_->y;
            if (dx * dx + dy * dy > 100 * 100) {
                pidState_.reset();
            }
        }
        else {
            pidState_.reset();
        }

        target_ = p;
    }

    // Wake worker so it begins tracking right away.
    cv_.notify_all();
}

void CursorAssistController::clearTarget() {
    // Clear target and reset dwell timer state.
    std::lock_guard<std::mutex> lk(mtx_);
    target_.reset();
    inRangeSince_ = {};
}

void CursorAssistController::threadMain() {
    using clock = std::chrono::steady_clock;

    // Timestamp used to compute dt between iterations.
    auto last = clock::now();

    while (running_.load(std::memory_order_relaxed)) {
        CursorAssistConfig cfgLocal;
        std::optional<CursorPoint> tgt;

        {
            // Wait briefly until:
            // - stop requested, OR
            // - enabled and a target exists (work to do)
            //
            // Note: we still run with a timeout so we can respond to stop quickly.
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait_for(lk, std::chrono::milliseconds(5), [&] {
                return !running_.load(std::memory_order_relaxed)
                    || (enabled_.load(std::memory_order_relaxed) && target_.has_value());
                });

            if (!running_.load(std::memory_order_relaxed)) break;

            // Copy shared state to locals to minimize time holding the lock.
            cfgLocal = cfg_;
            tgt = target_;
        }

        // If disabled or no target, idle lightly and reset timing baseline.
        if (!enabled_.load(std::memory_order_relaxed) || !tgt.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            last = clock::now();
            continue;
        }

        // Compute dt (seconds) since last tick.
        auto now = clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;

        // Prevent huge dt (e.g., if thread was stalled) from causing a big jump.
        // Also clamp the minimum dt to avoid division blow-ups in derivative term.
        dt = clampd(dt, 0.0005, 0.05);

        // Read current cursor position (screen coordinates).
        CursorPoint cur = backend_.getCursorPos();

        // Error in pixels (target - current).
        int ex = tgt->x - cur.x;
        int ey = tgt->y - cur.y;

        // Deadzone to reduce micro-jitter near the target.
        if (std::abs(ex) < cfgLocal.deadzone_px) ex = 0;
        if (std::abs(ey) < cfgLocal.deadzone_px) ey = 0;

        // PID output: desired relative movement per tick (in pixels).
        /*double ux = cfgLocal.pid.x.step((double)ex, dt);
        double uy = cfgLocal.pid.y.step((double)ey, dt);*/
        double ux, uy;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            // 注意：pidState_ 保存了状态，必须在锁内更新
            ux = pidState_.x.step((double)ex, dt);
            uy = pidState_.y.step((double)ey, dt);
        }

        // Round to integer pixel deltas.
        int dx = (int)std::lround(ux);
        int dy = (int)std::lround(uy);

        // Apply movement if non-zero.
        if (dx != 0 || dy != 0) {
            backend_.moveRelative(dx, dy);
        }

        // Optional dwell click:
        // - If within click_dist_px for dwell_ms
        // - And cooldown_ms since last click has elapsed
        if (cfgLocal.auto_click) {
            double dist = std::sqrt((double)ex * ex + (double)ey * ey);
            bool inRange = dist <= cfgLocal.click_dist_px;

            if (inRange) {
                // Start dwell timer when we first enter range.
                if (inRangeSince_.time_since_epoch().count() == 0) {
                    inRangeSince_ = now;
                }

                auto dwell = std::chrono::duration_cast<std::chrono::milliseconds>(now - inRangeSince_).count();
                auto cooldown = (lastClick_.time_since_epoch().count() == 0)
                    ? INT64_MAX
                    : std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClick_).count();

                if (dwell >= cfgLocal.dwell_ms && cooldown >= cfgLocal.cooldown_ms) {
                    backend_.leftClick();
                    lastClick_ = now;
                }
            }
            else {
                // Leaving range resets dwell timer.
                inRangeSince_ = {};
            }
        }

        // Simple rate limiting / CPU friendliness:
        // We do not try to hit tick_hz perfectly; we just keep it stable enough.
        int sleepUs = std::max(cfgLocal.min_sleep_us, (int)(1e6 / std::max(30, cfgLocal.tick_hz)));
        std::this_thread::sleep_for(std::chrono::microseconds(sleepUs));
    }
}