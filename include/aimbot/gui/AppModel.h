#pragma once

#include <opencv2/core.hpp>

#include "aimbot/app/AppConfig.h"
#include "aimbot/gui/Dx11MatTexture.h"

#include "aimbot/capture/DxgiDesktopDuplicationSource.h"
#include "aimbot/vision/HsvMasker.h"
#include "aimbot/vision/ContourDetector.h"
#include "aimbot/viz/OverlayRenderer.h"

#include "aimbot/lock/TargetLock.h"
#include "aimbot/lock/NearestToAimSelector.h"
#include "aimbot/lock/TargetContext.h"

#include "aimbot/cursor/CursorAssist.h"
#include "aimbot/cursor/Win32CursorBackend.h"

namespace aimbot::gui {

    struct GuiContext {
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
    };

    class AppModel {
    public:
        void init(const AppConfig& initial);
        void shutdown();

        // 每帧跑一次：capture->mask->detect->overlay->lock->cursor + upload textures
        void tick(const GuiContext& g);

        // 由 ControlPanel / hotkeys 触发
        void setAssistEnabled(bool en);
        void resetLock();

        // ConfigPanel 只设置 pending 标志；这里统一处理（先保留接口注释）
        void processPendingActions();

    public:
        // ---- config (runtime + ui) ----
        AppConfig cfg{};
        AppConfig uiCfg{};
        bool uiDirty = false;
        bool pendingApply = false;
        bool pendingSave = false;

        // ---- runtime toggles ----
        bool assistEnabled = false;

        // ---- metrics / status ----
        bool gotFrame = false;
        double latencyMs = 0.0;
        int detCount = 0;

        // ---- images ----
        cv::Mat screen; // BGR
        cv::Mat mask;   // GRAY

        // ---- GPU textures ----
        Dx11MatTexture screenTex;
        Dx11MatTexture maskTex;

    private:
        // ---- modules ----
        DxgiDesktopDuplicationSource source_;

        std::unique_ptr<HsvMasker> masker_;
        std::unique_ptr<ContourDetector> detector_;
        std::unique_ptr<OverlayRenderer> overlay_;

        std::unique_ptr<aimbot::lock::TargetLock> targetLock_;

        Win32CursorBackend backend_;
        std::unique_ptr<CursorAssistController> controller_;
    };

} // namespace aimbot::gui
