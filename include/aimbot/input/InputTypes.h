// input/InputTypes.h
#pragma once
#include <cstdint>

namespace aimbot::input {

    enum class DeviceType : uint8_t { Keyboard, Mouse, Gamepad };
    enum class TriggerType : uint8_t { Pressed, Released, Down };
    enum class BindingType : uint8_t { Button, Axis1D, Axis2D, Pointer };

    enum class Key : uint16_t {
        Unknown = 0,
        Escape,
        Q, E,
        // TODO: extend keyboard codes(A-Z, 0-9, F1..F12, etc.)
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
