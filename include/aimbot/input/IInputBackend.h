// input/IInputBackend.h
#pragma once
#include "aimbot/input/InputTypes.h"

namespace aimbot::input {

    class IInputBackend {
    public:
        virtual ~IInputBackend() = default;

        // 每帧调用一次：有的后端需要 message pump / cache update
        virtual void pump() = 0;

        // --- Digital ---
        virtual bool keyDown(Key k) const = 0;
        virtual bool mouseDown(MouseButton b) const = 0;
        virtual bool gamepadDown(int padIndex, GamepadButton b) const = 0;

        // --- Analog / Pointer (预留接口) ---
        // TODO: gamepad stick normalized to [-1,1]
        virtual Vec2 gamepadStick(int padIndex, bool leftStick) const { (void)padIndex; (void)leftStick; return {}; }

        // TODO: mouse wheel delta since last pump()
        virtual float mouseWheelDelta() const { return 0.0f; }

        // TODO: raw pointer delta since last pump()
        virtual Vec2 pointerDelta() const { return {}; }
    };

} // namespace
