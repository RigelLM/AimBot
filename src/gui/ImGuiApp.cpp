#include <chrono>
#include <cmath>
#include <windows.h>
#include <tchar.h>
#include <d3d11.h>
#include <dxgi.h>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

#include "aimbot/capture/DxgiDesktopDuplicationSource.h"
#include "aimbot/vision/HsvMasker.h"
#include "aimbot/vision/ContourDetector.h"
#include "aimbot/viz/OverlayRenderer.h"

#include "aimbot/lock/TargetLock.h"
#include "aimbot/lock/NearestToAimSelector.h"
#include "aimbot/lock/TargetContext.h"

#include "aimbot/cursor/CursorAssist.h"
#include "aimbot/cursor/Win32CursorBackend.h"

#include "aimbot/app/LoadAppConfig.h"

#include "aimbot/gui/ImGuiApp.h"
#include "aimbot/gui/Dx11MatTexture.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// TODO: Refactor into a reusable Input system
// Detect a key press on the rising edge (up -> down) using GetAsyncKeyState.
static bool keyPressedEdge(int vk, bool& prevDown) {
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = down && !prevDown;
    prevDown = down;
    return pressed;
}

namespace aimbot::gui {

    static ID3D11Device* g_pd3dDevice = nullptr;
    static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    static IDXGISwapChain* g_pSwapChain = nullptr;
    static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

    // ---- ImGui helpers to edit OpenCV-ish structs ----
    static bool DragDouble(const char* label, double* v,
        double speed, double v_min, double v_max,
        const char* fmt = "%.3f")
    {
        // DragScalar 的 v_speed 参数是 float，这里做一次安全转换
        float vs = (float)speed;
        return ImGui::DragScalar(label, ImGuiDataType_Double, v, vs, &v_min, &v_max, fmt);
    }

    static bool SliderDouble(const char* label, double* v,
        double v_min, double v_max,
        const char* fmt = "%.3f")
    {
        return ImGui::SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, fmt);
    }

    static bool EditHsvScalar3(const char* label, cv::Scalar& s, int min0, int max0, int min1, int max1, int min2, int max2)
    {
        int v[3] = { (int)s[0], (int)s[1], (int)s[2] };
        bool changed = ImGui::SliderInt3(label, v, 0, 255);
        // But HSV has different ranges: H[0,179], S/V[0,255]
        // So we clamp manually to desired ranges.
        v[0] = std::clamp(v[0], min0, max0);
        v[1] = std::clamp(v[1], min1, max1);
        v[2] = std::clamp(v[2], min2, max2);

        if (changed) {
            s = cv::Scalar(v[0], v[1], v[2]);
        }
        return changed;
    }
    
    static bool EditPoint2i(const char* label, cv::Point& p)
    {
        int v[2] = { p.x, p.y };
        bool changed = ImGui::DragInt2(label, v, 1.0f, -10000, 10000);
        if (changed) {
            p.x = v[0];
            p.y = v[1];
        }
        return changed;
    }

    static bool EditPIDAxis(const char* label, PIDAxis& a)
    {
        bool changed = false;
        if (ImGui::TreeNode(label))
        {
            changed |= DragDouble("kp", &a.kp, 0.001, 0.0, 10.0, "%.4f");
            changed |= DragDouble("ki", &a.ki, 0.001, 0.0, 10.0, "%.4f");
            changed |= DragDouble("kd", &a.kd, 0.001, 0.0, 10.0, "%.4f");

            ImGui::SeparatorText("Output clamp (per tick)");
            changed |= DragDouble("outMin", &a.outMin, 0.5, -5000.0, 0.0, "%.2f");
            changed |= DragDouble("outMax", &a.outMax, 0.5, 0.0, 5000.0, "%.2f");

            ImGui::SeparatorText("Integral clamp");
            changed |= DragDouble("iMin", &a.iMin, 1.0, -1e7, 0.0, "%.1f");
            changed |= DragDouble("iMax", &a.iMax, 1.0, 0.0, 1e7, "%.1f");

            ImGui::SeparatorText("Derivative filter");
            changed |= SliderDouble("dAlpha", &a.dAlpha, 0.0, 0.999, "%.3f");

            ImGui::TreePop();
        }
        return changed;
    }

    static bool DrawConfigUI(AppConfig& uiCfg)
    {
        bool changed = false;

        // ---------------- Capture ----------------
        if (ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragInt("offset_x", &uiCfg.capture.offset_x, 1.0f, -10000, 10000);
            changed |= ImGui::DragInt("offset_y", &uiCfg.capture.offset_y, 1.0f, -10000, 10000);
            ImGui::TextUnformatted("ROI -> Screen mapping: screen = roi + offset");
        }

        // ---------------- HSV ----------------
        if (ImGui::CollapsingHeader("HSV Mask", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted("OpenCV HSV ranges: H[0..179], S/V[0..255]");

            // Lower / Upper
            changed |= EditHsvScalar3("lower (H,S,V)", uiCfg.hsv.lower, 0, 179, 0, 255, 0, 255);
            changed |= EditHsvScalar3("upper (H,S,V)", uiCfg.hsv.upper, 0, 179, 0, 255, 0, 255);

            ImGui::Separator();
            changed |= ImGui::DragInt("minArea", &uiCfg.hsv.minArea, 10.0f, 0, 1000000);
            changed |= ImGui::DragInt("morphOpen", &uiCfg.hsv.morphOpen, 1.0f, 0, 99);
            changed |= ImGui::DragInt("morphClose", &uiCfg.hsv.morphClose, 1.0f, 0, 99);
            ImGui::TextUnformatted("morphOpen/Close = kernel size (0 disables)");
        }

        // ---------------- Lock ----------------
        if (ImGui::CollapsingHeader("Target Lock", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragFloat("match_radius", &uiCfg.lock.match_radius, 0.5f, 0.0f, 2000.0f, "%.1f");
            changed |= ImGui::DragInt("lost_frames_to_unlock", &uiCfg.lock.lost_frames_to_unlock, 1.0f, 0, 300);
        }

        // ---------------- Cursor Assist ----------------
        if (ImGui::CollapsingHeader("Cursor Assist", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragInt("tick_hz", &uiCfg.cursor.tick_hz, 1.0f, 1, 2000);
            changed |= ImGui::DragInt("deadzone_px", &uiCfg.cursor.deadzone_px, 1.0f, 0, 100);

            ImGui::SeparatorText("Auto click (dwell)");
            changed |= ImGui::Checkbox("auto_click", &uiCfg.cursor.auto_click);
            changed |= DragDouble("click_dist_px", &uiCfg.cursor.click_dist_px, 0.5, 0.0, 500.0, "%.1f");
            changed |= ImGui::DragInt("dwell_ms", &uiCfg.cursor.dwell_ms, 1.0f, 0, 10000);
            changed |= ImGui::DragInt("cooldown_ms", &uiCfg.cursor.cooldown_ms, 1.0f, 0, 60000);

            ImGui::SeparatorText("Worker loop");
            changed |= ImGui::DragInt("min_sleep_us", &uiCfg.cursor.min_sleep_us, 10.0f, 0, 2000000);

            ImGui::SeparatorText("PID");
            changed |= EditPIDAxis("PID X", uiCfg.cursor.pid.x);
            changed |= EditPIDAxis("PID Y", uiCfg.cursor.pid.y);

            ImGui::TextUnformatted("NOTE: Apply should reset PID internal state (integral/prevError).");
        }

        // ---------------- Overlay ----------------
        if (ImGui::CollapsingHeader("Overlay Style", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragInt("boxThickness", &uiCfg.overlay.boxThickness, 1.0f, 1, 20);
            changed |= ImGui::DragInt("centerRadius", &uiCfg.overlay.centerRadius, 1.0f, 0, 50);
            changed |= ImGui::DragInt("centerThickness", &uiCfg.overlay.centerThickness, 1.0f, -1, 20);

            changed |= DragDouble("fontScale", &uiCfg.overlay.fontScale, 0.01f, 0.1f, 3.0f, "%.2f");
            changed |= ImGui::DragInt("fontThickness", &uiCfg.overlay.fontThickness, 1.0f, 1, 10);

            changed |= EditPoint2i("latencyPos (x,y)", uiCfg.overlay.latencyPos);
            changed |= EditPoint2i("fpsPos (x,y)", uiCfg.overlay.fpsPos);

            ImGui::SeparatorText("Labels");
            changed |= ImGui::Checkbox("showConfidence", &uiCfg.overlay.showConfidence);
            changed |= ImGui::Checkbox("showArea", &uiCfg.overlay.showArea);
            changed |= ImGui::Checkbox("showIndex", &uiCfg.overlay.showIndex);
        }

        return changed;
    }

    static void CreateRenderTarget()
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer)
        {
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
    }

    static void CleanupRenderTarget()
    {
        if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    }

    static bool CreateDeviceD3D(HWND hWnd)
    {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

        HRESULT res = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createDeviceFlags,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &featureLevel,
            &g_pd3dDeviceContext);

        if (res != S_OK)
            return false;

        CreateRenderTarget();
        return true;
    }

    static void CleanupDeviceD3D()
    {
        CleanupRenderTarget();
        if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    }

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (::ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
            {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;

        case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // ban ALT application menu
                return 0;
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    int ImGuiApp::run()
    {
		// 1) Register win32 window class
        WNDCLASSEX wc = {
            sizeof(WNDCLASSEX),
            CS_CLASSDC,
            WndProc,
            0L,
            0L,
            GetModuleHandle(nullptr),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            _T("aimbot_imgui_dx11"),
            nullptr
        };
        ::RegisterClassEx(&wc);

		// 2) Create the application window
        HWND hwnd = ::CreateWindow(
            wc.lpszClassName,
            _T("aimbot - ImGui (Win32 + DX11)"),
            WS_OVERLAPPEDWINDOW,
            100, 100, 1280, 800,
            nullptr, nullptr, wc.hInstance, nullptr);

		// 3) Create Direct3D device
        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClass(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        // 4) Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // For Docking

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        // ---------------- Load config ----------------
        auto cfg = LoadAppConfigOrDefault("config/app.json");

        AppConfig uiCfg = cfg;     // UI editing copy
        bool uiDirty = false;      // true if uiCfg differs from the last-applied cfg (roughly)
        bool pendingApply = false; // Apply button pressed this frame
        bool pendingSave = false; // Save button pressed this frame

        // ---------------- Target lock ----------------
        aimbot::lock::TargetLock targetLock(
            cfg.lock,
            std::make_unique<aimbot::lock::NearestToAimSelector>()
        );

        // ---------------- Frame source ----------------
        DxgiDesktopDuplicationSource source;

        // ---------------- Vision ----------------
        HsvMasker masker(cfg.hsv);
        ContourDetector detector(cfg.hsv);
        OverlayRenderer overlay(cfg.overlay);

        // ---------------- Cursor assist ----------------
        Win32CursorBackend backend;
        CursorAssistController controller(backend);
        controller.updateConfig(cfg.cursor);
        controller.start();
        controller.setEnabled(false);

        bool assistEnabled = false;

        // TODO: Implement ROI capture
        // If you capture only a region of the screen (ROI), set these to the ROI's
        // top-left corner in screen coordinates. For full-screen capture, keep them at 0.
        const int CAPTURE_OFFSET_X = 0;
        const int CAPTURE_OFFSET_Y = 0;

        // ---------------- Mats ----------------
        cv::Mat screen; // BGR
        cv::Mat mask;   // GRAY (CV_8UC1)

        // ---------------- DX11 textures for ImGui ----------------
        Dx11MatTexture screenTex;
        Dx11MatTexture maskTex;

        // ---------------- Hotkey edge states ----------------
        bool prevQ = false, prevE = false, prevEsc = false;

        // Main loop
        bool done = false;
        while (!done)
        {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done) break;

            // Start the Dear ImGui frame
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // --------- UI ---------
            // --------- One frame pipeline: capture -> mask -> detect -> overlay -> lock -> cursor ---------
            auto frame_start = std::chrono::high_resolution_clock::now();

            bool got = source.grab(screen) && !screen.empty();
            if (got) {
                // 1) mask
                masker.run(screen, mask);

                // 2) detect
                auto dets = detector.detect(mask);

                // 3) latency + overlay
                auto frame_end = std::chrono::high_resolution_clock::now();
                double latency_ms =
                    std::chrono::duration<double, std::milli>(frame_end - frame_start).count();

                overlay.drawDetections(screen, dets);
                overlay.drawLatency(screen, latency_ms);

                // 4) hotkeys (keep your old behavior)
                if (keyPressedEdge(VK_ESCAPE, prevEsc)) {
                    PostQuitMessage(0);
                }
                if (keyPressedEdge('Q', prevQ)) {
                    assistEnabled = true;
                    controller.setEnabled(true);
                }
                if (keyPressedEdge('E', prevE)) {
                    assistEnabled = false;
                    controller.setEnabled(false);
                    controller.clearTarget();
                    targetLock.reset();
                }

                // 5) connect detections -> cursor assist
                if (assistEnabled) {
                    // Current cursor position in screen coordinates.
                    CursorPoint cur = backend.getCursorPos();

                    // Convert cursor position into *image coordinates* for target selection.
                    // (If capture is ROI, subtract the ROI offset.)
                    int mx_img = cur.x - CAPTURE_OFFSET_X;
                    int my_img = cur.y - CAPTURE_OFFSET_Y;

                    aimbot::lock::TargetContext tctx;
                    tctx.aimPointImg = cv::Point2f((float)mx_img, (float)my_img);
                    tctx.dt = 0.0f;

                    auto chosenOpt = targetLock.update(dets, tctx);

                    if (chosenOpt.has_value()) {
                        int chosen = *chosenOpt;

                        // Convert image coordinates back to monitor coordinates（ROI offset）
                        int tx = (int)std::lround(dets[chosen].center.x) + CAPTURE_OFFSET_X;
                        int ty = (int)std::lround(dets[chosen].center.y) + CAPTURE_OFFSET_Y;

                        controller.setTarget({ tx, ty });

                        // highlight lock target
                        cv::circle(screen,
                            cv::Point((int)dets[chosen].center.x, (int)dets[chosen].center.y),
                            12, cv::Scalar(0, 255, 255), 2);
                        cv::putText(screen, "LOCK",
                            cv::Point((int)dets[chosen].center.x + 14, (int)dets[chosen].center.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
                    }
                    else {
                        controller.clearTarget();
                    }
                }
                else {
                    controller.clearTarget();
                    targetLock.reset();
                }

                // 6) upload textures to GPU (screen BGR, mask GRAY)
                screenTex.update(g_pd3dDevice, g_pd3dDeviceContext, screen);
                maskTex.update(g_pd3dDevice, g_pd3dDeviceContext, mask);
            }
            else {
                // still process ESC even if no frame
                if (keyPressedEdge(VK_ESCAPE, prevEsc)) {
                    PostQuitMessage(0);
                }
            }

            ImGui::Begin("Config");

            // top buttons
            if (ImGui::Button("Apply")) {
                pendingApply = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                pendingSave = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Revert")) {
                uiCfg = cfg;
                uiDirty = false;
            }

            ImGui::SameLine();
            ImGui::Text("Dirty: %s", uiDirty ? "YES" : "NO");

            ImGui::Separator();

            // the real config UI
            uiDirty |= DrawConfigUI(uiCfg);

            ImGui::End();

            // --------- UI ---------
            ImGui::Begin("Control");
            ImGui::Text("Frame: %s", got ? "OK" : "NO");
            ImGui::Separator();

            if (ImGui::Button(assistEnabled ? "Disable (E)" : "Enable (Q)")) {
                assistEnabled = !assistEnabled;
                controller.setEnabled(assistEnabled);
                if (!assistEnabled) {
                    controller.clearTarget();
                    targetLock.reset();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Lock")) {
                controller.clearTarget();
                targetLock.reset();
            }

            ImGui::Text("Hotkeys: Q enable, E disable, ESC quit");
            ImGui::End();

            // Two view windows
            ImGui::Begin("Views");
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float halfW = (avail.x - 10.0f) * 0.5f; // gap

            // Left: Screen
            ImGui::BeginChild("ScreenChild", ImVec2(halfW, avail.y), true);
            ImGui::Text("Screen");
            if (screenTex.valid()) {
                float iw = (float)screenTex.width();
                float ih = (float)screenTex.height();
                float scale = halfW / iw;
                float showW = iw * scale;
                float showH = ih * scale;
                ImGui::Image((ImTextureID)screenTex.imTextureID(), ImVec2(showW, showH));
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // Right: Mask
            ImGui::BeginChild("MaskChild", ImVec2(halfW, avail.y), true);
            ImGui::Text("Mask");
            if (maskTex.valid()) {
                float iw = (float)maskTex.width();
                float ih = (float)maskTex.height();
                float scale = halfW / iw;
                float showW = iw * scale;
                float showH = ih * scale;
                ImGui::Image((ImTextureID)maskTex.imTextureID(), ImVec2(showW, showH));
            }
            ImGui::EndChild();

            ImGui::End();
            // -----------------------------------------------

            if (pendingApply) {
                pendingApply = false;

                // --- TODO: Apply contract ---
                // 1) Commit uiCfg -> cfg
                cfg = uiCfg;
                uiDirty = false;

                // 2) Apply cfg to runtime modules (NOT IMPLEMENTED YET)
                //    - masker.updateConfig(cfg.hsv);
                //    - detector.updateConfig(cfg.hsv);   // if your detector holds hsv config
                //    - targetLock.updateConfig(cfg.lock);
                //    - controller.updateConfig(cfg.cursor);
                //    - overlay.updateStyle(cfg.overlay);
                //
                // 3) Reset states as needed
                //    - targetLock.reset();
                //    - controller.clearTarget();
                //    - controller.resetPidState();  // or re-create internal PID controllers
                //
                // IMPORTANT: choose whether apply takes effect immediately mid-run
                // or only at "next frame" boundary (recommended).
            }

            if (pendingSave) {
                pendingSave = false;

                // --- TODO: Save contract ---
                // Implement a function like:
                //   bool SaveAppConfig(const std::string& path, const AppConfig& cfg, std::string* err);
                //
                // And call:
                //   SaveAppConfig("config/app.json", uiCfg, &err);
                //
                // Note: Decide whether Save writes uiCfg (current UI edits) or cfg (last applied).
                // Recommended: Save uiCfg (what user sees), independent of Apply.
            }

            ImGui::Render();

            const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            g_pSwapChain->Present(1, 0); // vsync
        }

		// 6) Cleanup
        controller.setEnabled(false);
        controller.clearTarget();
        controller.stop();

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);

        return 0;
    }

} // namespace aimbot::gui
