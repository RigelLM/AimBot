// main.cpp
// Simple PID cursor-assist demo:
// - Q: start drawing a circular target path around screen center
// - E: stop (disable controller and clear target)
// - ESC: exit program

#include <Windows.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "aimbot/cursor/CursorAssist.h"
#include "aimbot/cursor/Win32CursorBackend.h"

// Detect a key "press" on the rising edge (up -> down).
// vk: virtual-key code (e.g., VK_ESCAPE, 'Q', 'E')
// prevDown: previous key state; updated inside this function
static bool keyPressedEdge(int vk, bool& prevDown) {
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = down && !prevDown;
    prevDown = down;
    return pressed;
}

int main() {
    // Query primary display resolution and compute screen center.
    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    const int screenH = GetSystemMetrics(SM_CYSCREEN);
    const int cx = screenW / 2;
    const int cy = screenH / 2;

    // Backend performs actual cursor move/click via Win32 APIs.
    Win32CursorBackend backend;
    // Controller runs a high-frequency loop and applies PID to drive cursor toward target.
    CursorAssistController controller(backend);

    // Configure controller behavior.
    CursorAssistConfig cfg;
    cfg.tick_hz = 240;  // control loop frequency (Hz)
    cfg.deadzone_px = 2;    // ignore tiny errors to reduce jitter
    cfg.auto_click = false; // don't auto-click when reaching target

    // PID gains for X/Y axes (tune for smoothness vs responsiveness).
    cfg.pid.x.kp = 0.28; cfg.pid.x.ki = 0.00; cfg.pid.x.kd = 0.07;
    cfg.pid.y.kp = 0.28; cfg.pid.y.ki = 0.00; cfg.pid.y.kd = 0.07;
    // Output clamp (limits per-tick cursor movement commands).
    cfg.pid.x.outMin = -45; cfg.pid.x.outMax = 45;
    cfg.pid.y.outMin = -45; cfg.pid.y.outMax = 45;
    // Derivative low-pass filter factor (closer to 1 => stronger smoothing).
    cfg.pid.x.dAlpha = 0.85; cfg.pid.y.dAlpha = 0.85;

    // Apply config and start controller thread/loop.
    controller.updateConfig(cfg);
    controller.start();
    controller.setEnabled(false);   // start disabled until user presses Q

    std::cout << "Press Q to start drawing circle.\n"
        "Press E to stop.\n"
        "Press ESC to exit.\n";

    // Circle parameters: radius in pixels and angular speed.
    const double radius = 180.0;
    const double freqHz = 0.35; // revolutions per second
    const double omega = 2.0 * 3.14159265358979323846 * freqHz; // rad/s

    bool drawing = false;

    // Timestamp for when we started drawing (used to compute phase).
    auto t0 = std::chrono::steady_clock::now();

    // Previous key states for edge detection.
    bool prevQ = false, prevE = false, prevEsc = false;

    while (true) {
        if (keyPressedEdge(VK_ESCAPE, prevEsc)) {
            break;
        }

        if (keyPressedEdge('Q', prevQ)) {
            drawing = true;
            t0 = std::chrono::steady_clock::now();
            controller.setEnabled(true);
        }

        if (keyPressedEdge('E', prevE)) {
            drawing = false;
            controller.setEnabled(false);
            controller.clearTarget();
        }

        // If active, compute the current target point on the circle and send to controller.
        if (drawing) {
            auto now = std::chrono::steady_clock::now();
            double t = std::chrono::duration<double>(now - t0).count();
            double theta = omega * t;

            int tx = (int)std::lround((double)cx + radius * std::cos(theta));
            int ty = (int)std::lround((double)cy + radius * std::sin(theta));

            controller.setTarget({ tx, ty });
        }

        // Small sleep to reduce CPU usage; control loop runs independently at cfg.tick_hz.
        Sleep(5);
    }

    // Clean shutdown.
    controller.setEnabled(false);
    controller.clearTarget();
    controller.stop();

    return 0;
}
