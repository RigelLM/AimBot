// input/InputConfig.h
#pragma once

#include <string>
#include <vector>
#include <optional>

#include "aimbot/input/InputTypes.h"

namespace aimbot::input {

    enum class GamepadStick : uint8_t { LS, RS };
    enum class MouseWheelAxis : uint8_t { Vertical, Horizontal };

    struct ButtonBinding {
        DeviceType device{};
        TriggerType trigger{ TriggerType::Pressed };
        Key key{ Key::Unknown };
        MouseButton mouse{ MouseButton::Left };
        GamepadButton padBtn{ GamepadButton::A };
        int padIndex{ 0 };
    };

    struct Axis1DBinding {
        // TODO: implement Axis1D
        DeviceType device{};
        int padIndex{ 0 };
        std::optional<MouseWheelAxis> wheel;
        float deadzone{ 0.0f };
        float scale{ 1.0f };
    };

    struct Axis2DBinding {
        // TODO: implement Axis2D
        DeviceType device{};
        int padIndex{ 0 };
        std::optional<GamepadStick> stick;
        float deadzone{ 0.15f };
        float scale{ 1.0f };
    };

    struct PointerBinding {
        // TODO: implement PointerBinding
        // relative/absolute, smoothing, etc.
        float scale{ 1.0f };
    };

    struct Binding {
        std::string action;
        BindingType type{ BindingType::Button };

        ButtonBinding button{};

        Axis1DBinding axis1d{};
        Axis2DBinding axis2d{};
        PointerBinding pointer{};
    };

    struct InputConfig {
        std::vector<Binding> bindings;
    };

} // namespace
