#include <chrono>
#include <cmath>

#include "aimbot/gui/AppModel.h"

namespace aimbot::gui {

    void AppModel::init(const AppConfig& initial)
    {
        cfg = initial;
        uiCfg = cfg;
        uiDirty = false;
        pendingApply = pendingSave = false;

        masker_ = std::make_unique<HsvMasker>(cfg.hsv);
        detector_ = std::make_unique<ContourDetector>(cfg.hsv);
        overlay_ = std::make_unique<OverlayRenderer>(cfg.overlay);

        targetLock_ = std::make_unique<aimbot::lock::TargetLock>(
            cfg.lock,
            std::make_unique<aimbot::lock::NearestToAimSelector>()
        );

        controller_ = std::make_unique<CursorAssistController>(backend_);
        controller_->updateConfig(cfg.cursor);
        controller_->start();
        controller_->setEnabled(false);

        assistEnabled = false;
        gotFrame = false;
        latencyMs = 0.0;
        detCount = 0;
    }

    void AppModel::shutdown()
    {
        if (controller_) {
            controller_->setEnabled(false);
            controller_->clearTarget();
            controller_->stop();
            controller_.reset();
        }
        targetLock_.reset();
        overlay_.reset();
        detector_.reset();
        masker_.reset();
    }

    void AppModel::setAssistEnabled(bool en)
    {
        assistEnabled = en;
        if (controller_) controller_->setEnabled(en);

        if (!en) {
            if (controller_) controller_->clearTarget();
            if (targetLock_) targetLock_->reset();
        }
    }

    void AppModel::resetLock()
    {
        if (controller_) controller_->clearTarget();
        if (targetLock_) targetLock_->reset();
    }

    void AppModel::tick(const GuiContext& g)
    {
        auto frame_start = std::chrono::high_resolution_clock::now();

        gotFrame = source_.grab(screen) && !screen.empty();
        if (!gotFrame) return;

        // 1) mask
        masker_->run(screen, mask);

        // 2) detect
        auto dets = detector_->detect(mask);
        detCount = (int)dets.size();

        // 3) latency + overlay
        auto frame_end = std::chrono::high_resolution_clock::now();
        latencyMs = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();

        overlay_->drawDetections(screen, dets);
        overlay_->drawLatency(screen, latencyMs);

        // 4) lock + cursor
        if (assistEnabled && controller_ && targetLock_) {
            CursorPoint cur = backend_.getCursorPos();

            const int CAPTURE_OFFSET_X = cfg.capture.offset_x;
            const int CAPTURE_OFFSET_Y = cfg.capture.offset_y;

            int mx_img = cur.x - CAPTURE_OFFSET_X;
            int my_img = cur.y - CAPTURE_OFFSET_Y;

            aimbot::lock::TargetContext tctx;
            tctx.aimPointImg = cv::Point2f((float)mx_img, (float)my_img);
            tctx.dt = 0.0f;

            auto chosenOpt = targetLock_->update(dets, tctx);
            if (chosenOpt.has_value()) {
                int chosen = *chosenOpt;

                int tx = (int)std::lround(dets[chosen].center.x) + CAPTURE_OFFSET_X;
                int ty = (int)std::lround(dets[chosen].center.y) + CAPTURE_OFFSET_Y;
                controller_->setTarget({ tx, ty });

                cv::circle(screen,
                    cv::Point((int)dets[chosen].center.x, (int)dets[chosen].center.y),
                    12, cv::Scalar(0, 255, 255), 2);
                cv::putText(screen, "LOCK",
                    cv::Point((int)dets[chosen].center.x + 14, (int)dets[chosen].center.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
            }
            else {
                controller_->clearTarget();
            }
        }
        else {
            if (controller_) controller_->clearTarget();
            if (targetLock_) targetLock_->reset();
        }

        // 5) upload textures
        if (g.device && g.ctx) {
            screenTex.update(g.device, g.ctx, screen);
            maskTex.update(g.device, g.ctx, mask);
        }
    }

    void AppModel::processPendingActions()
    {
        if (pendingApply) {
            pendingApply = false;

            // TODO(apply contract):
            // 1) cfg = uiCfg; uiDirty=false;
            // 2) Apply to runtime modules:
            //    - masker_->config() = cfg.hsv;   或者写 updateConfig(cfg.hsv)
            //    - *detector_ = ContourDetector(cfg.hsv); (或 updateConfig)
            //    - overlay_ style update
            //    - targetLock_ update cfg.lock
            //    - controller_->updateConfig(cfg.cursor) 并重置 PID state
            // 3) reset runtime state:
            //    - resetLock();

            cfg = uiCfg;
            uiDirty = false;
        }

        if (pendingSave) {
            pendingSave = false;

            // TODO(save contract):
            // bool SaveAppConfig(const std::string& path, const AppConfig& cfg, std::string* err);
            // SaveAppConfig("config/app.json", uiCfg, &err);
        }
    }

} // namespace aimbot::gui
