// cursor/CursorAssist.h
// PID-based cursor assist controller.
//
// Design overview:
// - A platform-agnostic controller (CursorAssistController) runs a worker thread at tick_hz.
// - The controller reads the current cursor position via ICursorBackend.
// - Given a target point, it computes the error (target - cursor) and uses a 2D PID to
//   produce a bounded relative movement command (dx, dy) each tick.
// - Optional auto-click logic triggers a click when the cursor stays within a distance
//   threshold for a dwell time, with a cooldown between clicks.
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <condition_variable>
#include <cmath>
#include <algorithm>

#include "aimbot/app/AppConfig.h"

// Backend abstraction for cursor I/O.
// This keeps CursorAssistController platform-independent:
// - Windows: SendInput/GetCursorPos
// - Linux/X11/Wayland: different implementation
// - Tests: mock backend
class ICursorBackend {
public:
    virtual ~ICursorBackend() = default;
    // Return current cursor position in screen coordinates.
    virtual CursorPoint getCursorPos() = 0;
    // Move cursor by a relative delta (dx, dy) in pixels.
    virtual void moveRelative(int dx, int dy) = 0;
    // Perform a left mouse click.
    virtual void leftClick() = 0;
};

// Main controller class.
// Threading model:
// - start() spawns a worker thread that repeatedly:
//   1) reads target/config under mutex
//   2) reads cursor position from backend
//   3) computes PID output
//   4) issues backend movement/click commands
// - setTarget()/clearTarget()/updateConfig() are thread-safe via mutex + condition_variable.
class CursorAssistController {
public:
    // backend must outlive the controller.
    explicit CursorAssistController(ICursorBackend& backend);
    ~CursorAssistController();

    // Start/stop the worker thread.
    void start();
    void stop();

    // Enable/disable tracking. When disabled, the worker should not move/click.
    void setEnabled(bool en);
    bool isEnabled() const { return enabled_.load(std::memory_order_relaxed); }

    // Update configuration (thread-safe).
    void updateConfig(const CursorAssistConfig& cfg);

    // Get a copy of current configuration (thread-safe).
    CursorAssistConfig getConfigCopy() const;

    // Set/clear the target point (thread-safe).
    void setTarget(const CursorPoint& p);
    void clearTarget();

private:
    // Worker thread loop.
    void threadMain();

private:
    // Platform-specific cursor backend.
    ICursorBackend& backend_;

    // Mutex/condition_variable protect target_ and cfg_ and allow wakeups.
    mutable std::mutex mtx_;
    std::condition_variable cv_;

    // Thread control flags.
    std::atomic<bool> running_{ false };
    std::atomic<bool> enabled_{ false };

    // Tracking target; empty => no target.
    std::optional<CursorPoint> target_;     // guarded by mtx_

    // Current configuration (PID, timing, click settings).
    CursorAssistConfig cfg_;                // guarded by mtx_

    // Worker thread handle.
    std::thread worker_;

    // Auto-click timing state:
    // - inRangeSince_: when the cursor first entered the click distance
    // - lastClick_: last time a click was issued (for cooldown)
    std::chrono::steady_clock::time_point inRangeSince_{};
    std::chrono::steady_clock::time_point lastClick_{};
};
