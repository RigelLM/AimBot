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
			using K = aimbot::input::Key;

            // ---- Ranges first (compact) ----
            // Letters A-Z
            if (k >= K::A && k <= K::Z) {
                const int off = static_cast<int>(k) - static_cast<int>(K::A);
                return 'A' + off; // VK for letters is ASCII 'A'..'Z'
            }

            // Digits 0-9
            if (k >= K::D0 && k <= K::D9) {
                const int off = static_cast<int>(k) - static_cast<int>(K::D0);
                return '0' + off; // '0'..'9'
            }

            // Function keys F1-F12
            if (k >= K::F1 && k <= K::F12) {
                const int off = static_cast<int>(k) - static_cast<int>(K::F1);
                return VK_F1 + off; // VK_F1..VK_F12 are contiguous
            }

            // ---- Named keys / non-contiguous ----
            switch (k) {
            case K::Escape:    return VK_ESCAPE;
            case K::Enter:     return VK_RETURN;
            case K::Tab:       return VK_TAB;
            case K::Space:     return VK_SPACE;
            case K::Backspace: return VK_BACK;

            case K::Insert:    return VK_INSERT;
            case K::Delete:    return VK_DELETE;

            case K::LShift:    return VK_LSHIFT;
            case K::RShift:    return VK_RSHIFT;
            case K::LCtrl:     return VK_LCONTROL;
            case K::RCtrl:     return VK_RCONTROL;
            case K::LAlt:      return VK_LMENU;   // Alt == Menu
            case K::RAlt:      return VK_RMENU;

            case K::CapsLock:  return VK_CAPITAL;

            case K::Up:        return VK_UP;
            case K::Down:      return VK_DOWN;
            case K::Left:      return VK_LEFT;
            case K::Right:     return VK_RIGHT;

            case K::Home:      return VK_HOME;
            case K::End:       return VK_END;
            case K::PageUp:    return VK_PRIOR;   // PageUp
            case K::PageDown:  return VK_NEXT;    // PageDown

            default:           return 0;
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
