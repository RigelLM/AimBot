// capture/cursor/Win32CursorBackend.h
// Win32 implementation of the cursor backend used by CursorAssistController.
// Provides:
//   - Query current cursor position
//   - Move cursor by a relative delta (dx, dy)
//   - Perform a left mouse click
//
// Note: Uses SendInput (recommended modern Win32 input injection API).

#pragma once
#include "CursorAssist.h"
#include <Windows.h>

class Win32CursorBackend : public ICursorBackend {
public:
    // Get current cursor position in screen coordinates (pixels).
    CursorPoint getCursorPos() override {
        POINT p{};
        ::GetCursorPos(&p);
        return CursorPoint{ (int)p.x, (int)p.y };
    }

    // Move cursor relatively by(dx, dy) pixels.
    // This uses MOUSEEVENTF_MOVE with SendInput, which applies relative movement.
    void moveRelative(int dx, int dy) override {
        INPUT in{};
        in.type = INPUT_MOUSE;
        in.mi.dwFlags = MOUSEEVENTF_MOVE;
        in.mi.dx = dx;
        in.mi.dy = dy;
        ::SendInput(1, &in, sizeof(INPUT));
    }

    // Send a left-click (down + up).
    void leftClick() override {
        INPUT in[2]{};
        in[0].type = INPUT_MOUSE;
        in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        in[1].type = INPUT_MOUSE;
        in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        ::SendInput(2, in, sizeof(INPUT));
    }
};
