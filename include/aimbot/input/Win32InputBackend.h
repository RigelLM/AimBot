// input/Win32InputBackend.h
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Xinput.h>

#include "aimbot/input/IInputBackend.h"

namespace aimbot::input {

    class Win32InputBackend final : public IInputBackend {
    public:
        void pump() override {
            // TODO: If you later use RawInput / wheel accumulation, update caches here
        }

        bool keyDown(Key k) const override {
            const int vk = mapKeyToVK(k);
            if (!vk) return false;
            return (::GetAsyncKeyState(vk) & 0x8000) != 0;
        }

        bool mouseDown(MouseButton b) const override {
            const int vk = mapMouseToVK(b);
            if (!vk) return false;
            return (::GetAsyncKeyState(vk) & 0x8000) != 0;
        }

        bool gamepadDown(int padIndex, GamepadButton b) const override {
            XINPUT_STATE st{};
            if (::XInputGetState((DWORD)padIndex, &st) != ERROR_SUCCESS) return false;
            const WORD mask = mapPadButtonToMask(b);
            return (st.Gamepad.wButtons & mask) != 0;
        }

        // ---- TODO: (not implemented yet) ----
        // Vec2 gamepadStick(int padIndex, bool leftStick) const override { ... }
        // float mouseWheelDelta() const override { ... }
        // Vec2 pointerDelta() const override { ... }

    private:
        int mapKeyToVK(Key k) const {
            switch (k) {
            case Key::Escape: return VK_ESCAPE;
            case Key::Q:      return 'Q';
            case Key::E:      return 'E';
            default:          return 0;
            }
        }

        int mapMouseToVK(MouseButton b) const {
            switch (b) {
            case MouseButton::Left:   return VK_LBUTTON;
            case MouseButton::Right:  return VK_RBUTTON;
            case MouseButton::Middle: return VK_MBUTTON;
            case MouseButton::X1:     return VK_XBUTTON1;
            case MouseButton::X2:     return VK_XBUTTON2;
            }
            return 0;
        }

        WORD mapPadButtonToMask(GamepadButton b) const {
            switch (b) {
            case GamepadButton::A: return XINPUT_GAMEPAD_A;
            case GamepadButton::B: return XINPUT_GAMEPAD_B;
            case GamepadButton::X: return XINPUT_GAMEPAD_X;
            case GamepadButton::Y: return XINPUT_GAMEPAD_Y;
            case GamepadButton::LB: return XINPUT_GAMEPAD_LEFT_SHOULDER;
            case GamepadButton::RB: return XINPUT_GAMEPAD_RIGHT_SHOULDER;
            case GamepadButton::Back: return XINPUT_GAMEPAD_BACK;
            case GamepadButton::Start: return XINPUT_GAMEPAD_START;
            case GamepadButton::LStick: return XINPUT_GAMEPAD_LEFT_THUMB;
            case GamepadButton::RStick: return XINPUT_GAMEPAD_RIGHT_THUMB;
            case GamepadButton::DPadUp: return XINPUT_GAMEPAD_DPAD_UP;
            case GamepadButton::DPadDown: return XINPUT_GAMEPAD_DPAD_DOWN;
            case GamepadButton::DPadLeft: return XINPUT_GAMEPAD_DPAD_LEFT;
            case GamepadButton::DPadRight: return XINPUT_GAMEPAD_DPAD_RIGHT;
            default: return 0;
            }
        }
    };

} // namespace
