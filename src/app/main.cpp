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

#include "aimbot/app/LoadAppConfig.h"

// TODO: Refactor into a reusable Tracking module
// Try to associate current detections with the previously locked target based on center proximity.
//
// dets       : detections in the current frame
// prevCenter : locked center from last frame (or last known locked center)
// radiusPx   : max allowed distance for being considered the same target
//
// Returns:
//   index of the best matching detection in dets, or -1 if no match.
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

        // Pick the closest detection within the association radius.
        if (d2 <= r2 && d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }
    return best; // -1 means no "same target" found this frame.
}

// Pick a new target when no lock is held.
// Current policy: choose the detection whose center is closest to the current mouse position
// (in image coordinates).
static int pickNewTarget(const std::vector<Detection>& dets, int mx, int my) {
    if (dets.empty()) return -1;

    int bestIdx = -1;
    double bestD2 = std::numeric_limits<double>::infinity();

    for (int i = 0; i < (int)dets.size(); ++i) {
        double dx = dets[i].center.x - mx;
        double dy = dets[i].center.y - my;
        double d2 = dx * dx + dy * dy;

        // Optional filters you can enable:
        // if (dets[i].area < 1600) continue;
        // if (dets[i].confidence < 0.2f) continue;

        if (d2 < bestD2) {
            bestD2 = d2;
            bestIdx = i;
        }
    }
    return bestIdx;
}

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

    // Frame source (DXGI Desktop Duplication)
    DxgiDesktopDuplicationSource source;

    HsvMasker masker(cfg.hsv);

    // Contour-based detector operating on the binary mask.
    ContourDetector detector(cfg.hsv);

    // Overlay renderer for visualization (boxes/centers + latency text).
    OverlayRenderer overlay(cfg.overlay);

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

    // -------- Lock state --------
    bool hasLock = false;
    cv::Point2f lockCenter(0, 0);   // last known center of the locked target
    int missingFrames = 0;  // consecutive frames where locked target isn't found

    // Lock association parameters (tune to balance stickiness vs wrong associations).
    float lock_match_radius = cfg.lock.match_radius;    // pixels; larger => more "sticky" but more mismatch risk
	int lost_frames_to_unlock = cfg.lock.lost_frames_to_unlock; // frames; larger => more robust to missed detections

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
            hasLock = false;
            missingFrames = 0;
        }

        // -------- Connect detections to CursorAssist (core logic) --------
        if (assistEnabled) {
            // Current cursor position in screen coordinates.
            CursorPoint cur = backend.getCursorPos();

            // Convert cursor position into *image coordinates* for target selection.
            // (If capture is ROI, subtract the ROI offset.)
            int mx_img = cur.x - CAPTURE_OFFSET_X;
            int my_img = cur.y - CAPTURE_OFFSET_Y;
            int chosen = -1;

            if (hasLock) {
                // 1) Try to find the same locked target in the current detection set.
                chosen = matchLockedByCenter(dets, lockCenter, lock_match_radius);

                if (chosen >= 0) {
                    // Found: update the lock center and reset missing counter.
                    lockCenter = dets[chosen].center;
                    missingFrames = 0;
                }
                else {
                    // Not found: do not switch immediately; count consecutive misses.
                    missingFrames++;

                    if (missingFrames >= lost_frames_to_unlock) {
                        // Consider it truly gone: drop the lock.
                        hasLock = false;
                        missingFrames = 0;
                    }
                }
            }

            if (!hasLock) {
                // 2) Only when unlocked, pick a brand-new target (closest to cursor).
                chosen = pickNewTarget(dets, mx_img, my_img);

                if (chosen >= 0) {
                    hasLock = true;
                    lockCenter = dets[chosen].center;
                    missingFrames = 0;
                }
            }

            if (hasLock && chosen >= 0) {
                // 3) Convert the chosen target center back to screen coordinates and send to controller.
                int tx = (int)std::lround(dets[chosen].center.x) + CAPTURE_OFFSET_X;
                int ty = (int)std::lround(dets[chosen].center.y) + CAPTURE_OFFSET_Y;

                controller.setTarget({ tx, ty });

                // Visualization: highlight the locked target.
                cv::circle(screen,
                    cv::Point((int)dets[chosen].center.x, (int)dets[chosen].center.y),
                    12, cv::Scalar(0, 255, 255), 2);
                cv::putText(screen, "LOCK",
                    cv::Point((int)dets[chosen].center.x + 14, (int)dets[chosen].center.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
            }
            else {
                // No valid target: stop tracking this tick.
                controller.clearTarget();
            }
        }
        else
        {
            // Assist disabled: always clear tracking state.
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
    // Cleanup.
    controller.setEnabled(false);
    controller.clearTarget();
    controller.stop();

    cv::destroyAllWindows();

    return 0;
}
