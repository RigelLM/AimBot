// main.cpp
#include <Windows.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "aimbot/cursor/CursorAssist.h"
#include "aimbot/cursor/Win32CursorBackend.h"

static bool keyPressedEdge(int vk, bool& prevDown) {
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = down && !prevDown;
    prevDown = down;
    return pressed;
}

int main() {
    // --- 获取屏幕中心 ---
    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    const int screenH = GetSystemMetrics(SM_CYSCREEN);
    const int cx = screenW / 2;
    const int cy = screenH / 2;

    // --- 初始化控制器 ---
    Win32CursorBackend backend;
    CursorAssistController controller(backend);

    CursorAssistConfig cfg;
    cfg.tick_hz = 240;
    cfg.deadzone_px = 2;
    cfg.auto_click = false;          // 这里只画圈，不点击

    // PID 起点（可自行微调）
    cfg.pid.x.kp = 0.28; cfg.pid.x.ki = 0.00; cfg.pid.x.kd = 0.07;
    cfg.pid.y.kp = 0.28; cfg.pid.y.ki = 0.00; cfg.pid.y.kd = 0.07;
    cfg.pid.x.outMin = -45; cfg.pid.x.outMax = 45;
    cfg.pid.y.outMin = -45; cfg.pid.y.outMax = 45;
    cfg.pid.x.dAlpha = 0.85; cfg.pid.y.dAlpha = 0.85;

    controller.updateConfig(cfg);
    controller.start();
    controller.setEnabled(false);

    std::cout << "Press Q to start drawing circle.\n"
        "Press E to stop.\n"
        "Press ESC to exit.\n";

    // --- 画圈参数 ---
    const double radius = 180.0;              // 半径（像素）
    const double freqHz = 0.35;               // 圈速（每秒转几圈）
    const double omega = 2.0 * 3.14159265358979323846 * freqHz;

    bool drawing = false;

    // 记录“开始画圈”时的相位起点
    auto t0 = std::chrono::steady_clock::now();

    // 键位边沿检测
    bool prevQ = false, prevE = false, prevEsc = false;

    // 主循环：只负责更新 target（控制线程负责真正移动）
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

        if (drawing) {
            auto now = std::chrono::steady_clock::now();
            double t = std::chrono::duration<double>(now - t0).count();
            double theta = omega * t;

            int tx = (int)std::lround((double)cx + radius * std::cos(theta));
            int ty = (int)std::lround((double)cy + radius * std::sin(theta));

            controller.setTarget({ tx, ty });
        }

        // 主线程更新目标点频率不需要特别高，60~200Hz 都行
        Sleep(5);
    }

    // 退出清理
    controller.setEnabled(false);
    controller.clearTarget();
    controller.stop();

    return 0;
}
