#pragma once
#include "CursorAssist.h"
#include <Windows.h>

class Win32CursorBackend : public ICursorBackend {
public:
    CursorPoint getCursorPos() override {
        POINT p{};
        ::GetCursorPos(&p);
        return CursorPoint{ (int)p.x, (int)p.y };
    }

    void moveRelative(int dx, int dy) override {
        INPUT in{};
        in.type = INPUT_MOUSE;
        in.mi.dwFlags = MOUSEEVENTF_MOVE;
        in.mi.dx = dx;
        in.mi.dy = dy;
        ::SendInput(1, &in, sizeof(INPUT));
    }

    void leftClick() override {
        INPUT in[2]{};
        in[0].type = INPUT_MOUSE;
        in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        in[1].type = INPUT_MOUSE;
        in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        ::SendInput(2, in, sizeof(INPUT));
    }
};
