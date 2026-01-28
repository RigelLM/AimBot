// End-to-end demo: desktop capture -> HSV mask -> contour detections -> overlay -> cursor assist PID.
//
// Hotkeys:
//   Q   : enable assist (track + optional dwell click)
//   E   : disable assist (stop tracking, clear target, unlock)
//   ESC : quit
//
// Locking strategy:
// - Once a target is selected ("locked"), we try to keep tracking the *same* target by
//   matching detections whose center stays within LOCK_MATCH_RADIUS of the previous locked center.
// - We do NOT switch targets immediately if the locked target is missing for a frame.
//   Instead we count consecutive missing frames; only after LOST_FRAMES_TO_UNLOCK frames
//   do we drop the lock and choose a new target.
//
// Coordinate notes:
// - If you capture a ROI instead of full screen, you must provide CAPTURE_OFFSET_X/Y which
//   maps image coordinates -> screen coordinates (screen = image + offset).

#include <chrono>
#include <iostream>
#include <limits>
#include <cmath>

#include <opencv2/opencv.hpp>

#include "aimbot/capture/DxgiDesktopDuplicationSource.h"
#include "aimbot/vision/HsvMasker.h"
#include "aimbot/vision/ContourDetector.h"
#include "aimbot/viz/OverlayRenderer.h"

#include "aimbot/cursor/CursorAssist.h"
#include "aimbot/cursor/Win32CursorBackend.h"

#include "aimbot/lock/TargetLock.h"
#include "aimbot/lock/NearestToAimSelector.h"
#include "aimbot/lock/TargetContext.h"

#include "aimbot/app/LoadAppConfig.h"

// TODO: Refactor into a reusable Input system
// Detect a key press on the rising edge (up -> down) using GetAsyncKeyState.
static bool keyPressedEdge(int vk, bool& prevDown) {
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = down && !prevDown;
    prevDown = down;
    return pressed;
}

int main() {
	auto cfg = LoadAppConfigOrDefault("config/app.json");

    aimbot::lock::TargetLock targetLock(
        cfg.lock,
        std::make_unique<aimbot::lock::NearestToAimSelector>()
    );

    // Frame source (DXGI Desktop Duplication)
    DxgiDesktopDuplicationSource source;

    HsvMasker masker(cfg.hsv);

    // Contour-based detector operating on the binary mask.
    ContourDetector detector(cfg.hsv);

    // Overlay renderer for visualization (boxes/centers + latency text).
    OverlayRenderer overlay(cfg.overlay);

    // TODO: Implement GUI for this app
    // Debug windows.
    cv::namedWindow("Screen Capture", cv::WINDOW_KEEPRATIO);
    cv::namedWindow("Mask", cv::WINDOW_KEEPRATIO);

    cv::Mat screen;
    cv::Mat mask;

    // -------- Cursor assist controller initialization --------
     // Win32 backend provides GetCursorPos/SendInput.
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

    std::cout << "[Q] enable assist (track + dwell click)\n"
        "[E] disable assist\n"
        "[ESC] quit\n";

    // Key edge state for hotkeys.
    bool prevQ = false, prevE = false, prevEsc = false;

    while (true) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Grab a new frame (may return false if no new frame / timeout)
        if (!source.grab(screen) || screen.empty()) {
            int key = cv::waitKey(1);
            if (key == 27) break; // ESC
            continue;
        }

        // BGR -> HSV -> inRange => mask
        masker.run(screen, mask);

        // Detect candidates from mask.
        auto dets = detector.detect(mask);

        // Frame-to-frame latency measurement.
        auto frame_end = std::chrono::high_resolution_clock::now();
        double latency_ms =
            std::chrono::duration<double, std::milli>(frame_end - frame_start).count();

        // Draw detection overlays and latency.
        overlay.drawDetections(screen, dets);
        overlay.drawLatency(screen, latency_ms);

        // -------- Read hotkeys --------
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
            targetLock.reset();
        }

        // -------- Connect detections to CursorAssist (core logic) --------
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

                // Visualization: highlight the locked/current target.
                cv::circle(screen,
                    cv::Point((int)dets[chosen].center.x, (int)dets[chosen].center.y),
                    12, cv::Scalar(0, 255, 255), 2);
                cv::putText(screen, "LOCK",
                    cv::Point((int)dets[chosen].center.x + 14, (int)dets[chosen].center.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
            }
            else {
				// No available target in this frame：could be temporarily lost (still locked), could also be fully unlocked.
                controller.clearTarget();

                // Optional：If you want to show“LOST n”
                // if (targetLock.isLocked()) {
                //     cv::putText(screen, ("LOST " + std::to_string(targetLock.lostFrames())).c_str(),
                //         cv::Point(20, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                // }
            }
        }
        else
        {
            // Assist disabled: always clear tracking state.
            controller.clearTarget();
			targetLock.reset();
        }

        // Show windows
        cv::imshow("Mask", mask);
        cv::imshow("Screen Capture", screen);

        // Pump OpenCV window events
        cv::waitKey(1);
    }
    // Cleanup.
    controller.setEnabled(false);
    controller.clearTarget();
    controller.stop();

    cv::destroyAllWindows();

    return 0;
}
