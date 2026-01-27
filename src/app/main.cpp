#include <chrono>
#include <iostream>
#include <limits>
#include <cmath>

#include <opencv2/opencv.hpp>

#include "aimbot/capture/DxgiDesktopDuplicationSource.h"
#include "aimbot/vision/HsvConfig.h"
#include "aimbot/vision/HsvMasker.h"
#include "aimbot/vision/ContourDetector.h"
#include "aimbot/viz/OverlayRenderer.h"

#include "aimbot/cursor/CursorAssist.h"
#include "aimbot/cursor/Win32CursorBackend.h"

static int matchLockedByCenter(const std::vector<Detection>& dets,
    const cv::Point2f& prevCenter,
    float radiusPx)
{
    if (dets.empty()) return -1;

    const float r2 = radiusPx * radiusPx;
    int best = -1;
    float bestD2 = (std::numeric_limits<float>::max)();

    for (int i = 0; i < (int)dets.size(); ++i) {
        float dx = dets[i].center.x - prevCenter.x;
        float dy = dets[i].center.y - prevCenter.y;
        float d2 = dx * dx + dy * dy;

        if (d2 <= r2 && d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }
    return best; // -1 表示没找到“同一个”目标
}

static int pickNewTarget(const std::vector<Detection>& dets, int mx, int my) {
    if (dets.empty()) return -1;

    int bestIdx = -1;
    double bestD2 = std::numeric_limits<double>::infinity();

    for (int i = 0; i < (int)dets.size(); ++i) {
        double dx = dets[i].center.x - mx;
        double dy = dets[i].center.y - my;
        double d2 = dx * dx + dy * dy;

        // 可选过滤：
        // if (dets[i].area < 1600) continue;
        // if (dets[i].confidence < 0.2f) continue;

        if (d2 < bestD2) {
            bestD2 = d2;
            bestIdx = i;
        }
    }
    return bestIdx;
}

static bool keyPressedEdge(int vk, bool& prevDown) {
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = down && !prevDown;
    prevDown = down;
    return pressed;
}

int main() {
    // Step1: Frame source (DXGI Desktop Duplication)
    DxgiDesktopDuplicationSource source;

    // Step2: HSV masker config (use your old thresholds)
    HsvConfig cfg;
    cfg.lower = cv::Scalar(82, 199, 118);
    cfg.upper = cv::Scalar(97, 255, 255);
    cfg.minArea = 1600;     // Step3: same as your old (r.width*r.height < 1600) filter
    cfg.morphOpen = 0;      // keep 0 for equivalent behavior
    cfg.morphClose = 0;

    HsvMasker masker(cfg);

    // Step3: contour detector
    ContourDetector detector(cfg);

    // Step4: overlay renderer
    OverlayStyle style;
    style.showConfidence = true;
    style.showArea = false;
    style.showIndex = false;
    OverlayRenderer overlay(style);

    cv::namedWindow("Screen Capture", cv::WINDOW_KEEPRATIO);
    cv::namedWindow("Mask", cv::WINDOW_KEEPRATIO);

    cv::Mat screen;
    cv::Mat mask;

    // --- 初始化控制器 ---
    Win32CursorBackend backend;
    CursorAssistController controller(backend);

    CursorAssistConfig cconf;
    cconf.tick_hz = 240;
    cconf.deadzone_px = 2;

    // 开启 dwell click
    cconf.auto_click = true;
    cconf.click_dist_px = 16.0;  // 进入这个距离就开始计时
    cconf.dwell_ms = 150;        // 停留 150ms 点击
    cconf.cooldown_ms = 150;     // 点完 150ms 内不再点

    // PID 初值（可调）
    cconf.pid.x.kp = 0.28; cconf.pid.x.ki = 0.00; cconf.pid.x.kd = 0.07;
    cconf.pid.y.kp = 0.28; cconf.pid.y.ki = 0.00; cconf.pid.y.kd = 0.07;
    cconf.pid.x.outMin = -45; cconf.pid.x.outMax = 45;
    cconf.pid.y.outMin = -45; cconf.pid.y.outMax = 45;
    cconf.pid.x.dAlpha = 0.85; cconf.pid.y.dAlpha = 0.85;

    controller.updateConfig(cconf);
    controller.start();
    controller.setEnabled(false);

    bool assistEnabled = false;

    // 如果你抓的是 ROI，把它填成 ROI 的左上角屏幕坐标；全屏抓取则为 0
    const int CAPTURE_OFFSET_X = 0;
    const int CAPTURE_OFFSET_Y = 0;

    std::cout << "[Q] enable assist (track + dwell click)\n"
        "[E] disable assist\n"
        "[ESC] quit\n";

    // 键位边沿检测
    bool prevQ = false, prevE = false, prevEsc = false;

    // -------- Lock state --------
    bool hasLock = false;
    cv::Point2f lockCenter(0, 0);      // 记录锁定目标的“上一帧中心”
    int missingFrames = 0;            // 连续丢失计数

    // 参数：你可以调
    const float LOCK_MATCH_RADIUS = 60.0f;  // 关联半径（像素），越大越容易“粘住”但也更可能粘错
    const int   LOST_FRAMES_TO_UNLOCK = 5;  // 连续丢失多少帧才算真正消失（抗漏检）

    while (true) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Grab a new frame (may return false if no new frame / timeout)
        if (!source.grab(screen) || screen.empty()) {
            int key = cv::waitKey(1);
            if (key == 27) break; // ESC
            continue;
        }

        // Step2: BGR -> HSV -> inRange => mask
        masker.run(screen, mask);

        // Step3: detect from mask
        auto dets = detector.detect(mask);

        // latency
        auto frame_end = std::chrono::high_resolution_clock::now();
        double latency_ms =
            std::chrono::duration<double, std::milli>(frame_end - frame_start).count();

        // Step4: draw overlay on the original screen frame
        overlay.drawDetections(screen, dets);
        overlay.drawLatency(screen, latency_ms);

        // ---------------- 读取按键 ----------------
        if (keyPressedEdge(VK_ESCAPE, prevEsc)) {
            break;
        }

        if (keyPressedEdge('Q', prevQ)) {
            assistEnabled = true;
            controller.setEnabled(true);
        }

        if (keyPressedEdge('E', prevE)) {
            assistEnabled = false;
            controller.setEnabled(false);
            controller.clearTarget();
            hasLock = false;
            missingFrames = 0;
        }

        // ---------------- 把 dets 接到 CursorAssist：核心逻辑 ----------------
        if (assistEnabled) {
            CursorPoint cur = backend.getCursorPos();

            // 注意：如果你的 capture 是 ROI，需要把鼠标坐标减 offset，再拿去做选择
            int mx_img = cur.x - CAPTURE_OFFSET_X;
            int my_img = cur.y - CAPTURE_OFFSET_Y;
            int chosen = -1;

            if (hasLock) {
                // 1) 尝试在当前 dets 中找到“同一个锁定目标”
                chosen = matchLockedByCenter(dets, lockCenter, LOCK_MATCH_RADIUS);

                if (chosen >= 0) {
                    // 找到了：更新 lockCenter，清空丢失计数
                    lockCenter = dets[chosen].center;
                    missingFrames = 0;
                }
                else {
                    // 没找到：先不立刻切换，累计丢失帧数
                    missingFrames++;

                    if (missingFrames >= LOST_FRAMES_TO_UNLOCK) {
                        // 2) 锁定目标“确认消失” -> 解锁
                        hasLock = false;
                        missingFrames = 0;
                    }
                }
            }

            if (!hasLock) {
                // 3) 只有在无锁定时，才选择一个新目标并上锁
                chosen = pickNewTarget(dets, mx_img, my_img);

                if (chosen >= 0) {
                    hasLock = true;
                    lockCenter = dets[chosen].center;
                    missingFrames = 0;
                }
            }

            if (hasLock && chosen >= 0) {
                // 4) 把锁定目标 center 转成屏幕坐标，喂给 CursorAssist
                int tx = (int)std::lround(dets[chosen].center.x) + CAPTURE_OFFSET_X;
                int ty = (int)std::lround(dets[chosen].center.y) + CAPTURE_OFFSET_Y;

                controller.setTarget({ tx, ty });

                // 可视化：标出锁定目标
                cv::circle(screen,
                    cv::Point((int)dets[chosen].center.x, (int)dets[chosen].center.y),
                    12, cv::Scalar(0, 255, 255), 2);
                cv::putText(screen, "LOCK",
                    cv::Point((int)dets[chosen].center.x + 14, (int)dets[chosen].center.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
            }
            else {
                // 没目标/未锁定：不追踪
                controller.clearTarget();
            }
        }
        else
        {
            // assist 未启用：清空追踪 & 清锁
            controller.clearTarget();
            hasLock = false;
            missingFrames = 0;
        }

        // Show windows
        cv::imshow("Mask", mask);
        cv::imshow("Screen Capture", screen);

        // Pump OpenCV window events
		cv::waitKey(1);
    }

    cv::destroyAllWindows();
    return 0;
}
