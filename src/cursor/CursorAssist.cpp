#include "aimbot/cursor/CursorAssist.h"

static inline double clampd(double v, double lo, double hi) {
    return std::max(lo, std::min(v, hi));
}

CursorAssistController::CursorAssistController(ICursorBackend& backend)
    : backend_(backend) {
}

CursorAssistController::~CursorAssistController() {
    stop();
}

void CursorAssistController::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;

    worker_ = std::thread(&CursorAssistController::threadMain, this);
}

void CursorAssistController::stop() {
    if (!running_.exchange(false)) return;
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void CursorAssistController::setEnabled(bool en) {
    enabled_.store(en, std::memory_order_relaxed);
    cv_.notify_all();
}

void CursorAssistController::updateConfig(const CursorAssistConfig& cfg) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        cfg_ = cfg;
        cfg_.pid.reset();
    }
    cv_.notify_all();
}

CursorAssistConfig CursorAssistController::getConfigCopy() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return cfg_;
}

void CursorAssistController::setTarget(const CursorPoint& p) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        target_ = p;
    }
    cv_.notify_all();
}

void CursorAssistController::clearTarget() {
    std::lock_guard<std::mutex> lk(mtx_);
    target_.reset();
    inRangeSince_ = {};
}

void CursorAssistController::threadMain() {
    using clock = std::chrono::steady_clock;

    auto last = clock::now();

    while (running_.load(std::memory_order_relaxed)) {
        CursorAssistConfig cfgLocal;
        std::optional<CursorPoint> tgt;

        {
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait_for(lk, std::chrono::milliseconds(5), [&] {
                return !running_.load(std::memory_order_relaxed)
                    || (enabled_.load(std::memory_order_relaxed) && target_.has_value());
                });

            if (!running_.load(std::memory_order_relaxed)) break;

            cfgLocal = cfg_;
            tgt = target_;
        }

        if (!enabled_.load(std::memory_order_relaxed) || !tgt.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            last = clock::now();
            continue;
        }

        auto now = clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;

        // 防止卡顿导致 dt 过大，一步飞出去
        dt = clampd(dt, 0.0005, 0.05);

        CursorPoint cur = backend_.getCursorPos();
        int ex = tgt->x - cur.x;
        int ey = tgt->y - cur.y;

        // 死区抑制微抖
        if (std::abs(ex) < cfgLocal.deadzone_px) ex = 0;
        if (std::abs(ey) < cfgLocal.deadzone_px) ey = 0;

        // PID 输出（每 tick 相对移动多少像素）
        double ux = cfgLocal.pid.x.step((double)ex, dt);
        double uy = cfgLocal.pid.y.step((double)ey, dt);

        int dx = (int)std::lround(ux);
        int dy = (int)std::lround(uy);

        if (dx != 0 || dy != 0) {
            backend_.moveRelative(dx, dy);
        }

        // dwell click（可选）
        if (cfgLocal.auto_click) {
            double dist = std::sqrt((double)ex * ex + (double)ey * ey);
            bool inRange = dist <= cfgLocal.click_dist_px;

            if (inRange) {
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
                inRangeSince_ = {};
            }
        }

        // 固定频率节流（不要求特别精确，但要稳定）
        int sleepUs = std::max(cfgLocal.min_sleep_us, (int)(1e6 / std::max(30, cfgLocal.tick_hz)));
        std::this_thread::sleep_for(std::chrono::microseconds(sleepUs));
    }
}
