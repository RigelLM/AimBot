// capture/CursorAssist.h
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

// Single-axis PID controller with:
// - derivative low-pass filtering (dAlpha)
// - output saturation (outMin/outMax)
// - integral clamping (iMin/iMax)
// - simple anti-windup: do not integrate if saturated and error pushes further into saturation
struct PIDAxis {
    // PID gains
    double kp{ 0 }, ki{ 0 }, kd{ 0 };
    // Internal state
    double integral{ 0 }, prevError{ 0 }, prevDeriv{ 0 };

    // Output clamp (limits the command magnitude)
    double outMin{ -40 }, outMax{ 40 };
    // Integral clamp (prevents integral windup)
    double iMin{ -2000 }, iMax{ 2000 };
    // Derivative filter coefficient in [0,1):
    //   derivFiltered = dAlpha * prevDeriv + (1 - dAlpha) * derivRaw
    double dAlpha{ 0.85 };

    // Advance one PID step given current error e and time step dt (seconds).
    // Returns the saturated control output u.
    double step(double e, double dt) {
        if (dt <= 0) return 0.0;

        // Raw derivative of error
        double deriv = (e - prevError) / dt;
        // Low-pass filter derivative to reduce noise/jitter
        deriv = dAlpha * prevDeriv + (1.0 - dAlpha) * deriv;

        // Candidate integral update
        double integralCandidate = integral + e * dt;

        // Unsaturated control output
        double uUnsat = kp * e + ki * integralCandidate + kd * deriv;
        // Saturate output
        double u = (std::max)(outMin, (std::min)(uUnsat, outMax));

        // Anti-windup:
        // If saturated and the error would push further into saturation, skip integrating.
        bool saturated = (u != uUnsat);
        bool pushingFurther =
            (u == outMax && e > 0) ||
            (u == outMin && e < 0);

        if (!(saturated && pushingFurther)) {
            // Commit integral update with clamping
            integral = (std::max)(iMin, (std::min)(integralCandidate, iMax));
        }

        // Update state
        prevError = e;
        prevDeriv = deriv;
        return u;
    }

    // Reset controller memory/state.
    void reset() { integral = 0; prevError = 0; prevDeriv = 0; }
};

// Two-axis PID (X and Y share the same structure but independent state).
struct PID2D {
    PIDAxis x;
    PIDAxis y;
    void reset() { x.reset(); y.reset(); }
};

// Simple integer point in screen pixel coordinates.
struct CursorPoint {
    int x{ 0 };
    int y{ 0 };
};

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

// Configuration for the cursor assist controller.
struct CursorAssistConfig {
    // Control loop frequency (ticks per second).
    int tick_hz = 240;

    // Deadzone radius (pixels): if |error| is small, do not move to reduce jitter.
    int deadzone_px = 2;

    // Auto-click feature:
    // If enabled, click when the cursor stays within click_dist_px for dwell_ms,
    // then enforce cooldown_ms before the next click.
    bool auto_click = false;
    double click_dist_px = 18.0;
    int dwell_ms = 120;
    int cooldown_ms = 400;

    // Minimum sleep (microseconds) in the worker loop to avoid busy-waiting.
    int min_sleep_us = 500;

    // Default PID gains and limits for X/Y axes.
    PID2D pid{
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85},
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85}
    };
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
