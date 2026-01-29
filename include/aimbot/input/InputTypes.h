// input/InputTypes.h
#pragma once
#include <cstdint>

namespace aimbot::input {

    enum class DeviceType : uint8_t { Keyboard, Mouse, Gamepad };
    enum class TriggerType : uint8_t { Pressed, Released, Down };
    enum class BindingType : uint8_t { Button, Axis1D, Axis2D, Pointer };

    enum class Key : uint16_t {
        Unknown = 0,

        // --- Common ---
        Escape,
        Enter,
        Tab,
        Space,
        Backspace,
        Delete,
        Insert,

        // --- Modifiers ---
        LShift, RShift,
        LCtrl, RCtrl,
        LAlt, RAlt,
        CapsLock,

        // --- Navigation ---
        Up, Down, Left, Right,
        Home, End,
        PageUp, PageDown,

        // --- Function keys ---
        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,

        // --- Digits ---
        D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,

        // --- Letters ---
        A, B, C, D, E, F, G, H, I, J,
        K, L, M, N, O, P, Q, R, S, T,
        U, V, W, X, Y, Z,
    };

    enum class MouseButton : uint8_t { Left, Right, Middle, X1, X2 };

    enum class GamepadButton : uint16_t {
        A, B, X, Y,
        LB, RB,
        Back, Start,
        LStick, RStick,
        DPadUp, DPadDown, DPadLeft, DPadRight,
        // TODO: extend gamepad button codes
    };

    struct Vec2 { float x{}, y{}; };

} // namespace
