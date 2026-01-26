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

struct PIDAxis {
    double kp{ 0 }, ki{ 0 }, kd{ 0 };
    double integral{ 0 }, prevError{ 0 }, prevDeriv{ 0 };

    double outMin{ -40 }, outMax{ 40 };      // 每 tick 最大移动像素
    double iMin{ -2000 }, iMax{ 2000 };      // 积分限幅
    double dAlpha{ 0.85 };                 // 导数低通滤波 (0~1)

    double step(double e, double dt) {
        if (dt <= 0) return 0.0;

        double deriv = (e - prevError) / dt;
        deriv = dAlpha * prevDeriv + (1.0 - dAlpha) * deriv;

        double integralCandidate = integral + e * dt;

        double uUnsat = kp * e + ki * integralCandidate + kd * deriv;
        double u = std::max(outMin, std::min(uUnsat, outMax));

        bool saturated = (u != uUnsat);
        bool pushingFurther =
            (u == outMax && e > 0) ||
            (u == outMin && e < 0);

        if (!(saturated && pushingFurther)) {
            integral = std::max(iMin, std::min(integralCandidate, iMax));
        }

        prevError = e;
        prevDeriv = deriv;
        return u;
    }

    void reset() { integral = 0; prevError = 0; prevDeriv = 0; }
};

struct PID2D {
    PIDAxis x;
    PIDAxis y;
    void reset() { x.reset(); y.reset(); }
};

struct CursorPoint {
    int x{ 0 };
    int y{ 0 };
};

class ICursorBackend {
public:
    virtual ~ICursorBackend() = default;
    virtual CursorPoint getCursorPos() = 0;
    virtual void moveRelative(int dx, int dy) = 0;
    virtual void leftClick() = 0;
};

struct CursorAssistConfig {
    // 控制线程频率
    int tick_hz = 240;

    // 死区：误差很小时不动，避免抖
    int deadzone_px = 2;

    // 点击策略（可访问性常用 dwell click）
    bool auto_click = false;
    double click_dist_px = 18.0;
    int dwell_ms = 120;
    int cooldown_ms = 400;

    // 安全：每 tick 最小 sleep，防止 CPU 拉满
    int min_sleep_us = 500;

    // PID 参数（默认给一个能跑的起点）
    PID2D pid{
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85},
        PIDAxis{0.25, 0.00, 0.06, 0,0,0,  -40, 40,  -2000, 2000, 0.85}
    };
};

class CursorAssistController {
public:
    explicit CursorAssistController(ICursorBackend& backend);
    ~CursorAssistController();

    void start();
    void stop();

    void setEnabled(bool en);
    bool isEnabled() const { return enabled_.load(std::memory_order_relaxed); }

    void updateConfig(const CursorAssistConfig& cfg);
    CursorAssistConfig getConfigCopy() const;

    // 目标点：不断更新即可（例如每帧视觉输出）
    void setTarget(const CursorPoint& p);
    void clearTarget();

private:
    void threadMain();

private:
    ICursorBackend& backend_;

    mutable std::mutex mtx_;
    std::condition_variable cv_;

    std::atomic<bool> running_{ false };
    std::atomic<bool> enabled_{ false };

    std::optional<CursorPoint> target_;     // guarded by mtx_
    CursorAssistConfig cfg_;                // guarded by mtx_

    std::thread worker_;

    // dwell/cooldown 状态
    std::chrono::steady_clock::time_point inRangeSince_{};
    std::chrono::steady_clock::time_point lastClick_{};
};
