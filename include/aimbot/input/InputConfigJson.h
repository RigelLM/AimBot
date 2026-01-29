#pragma once

#include <string>
#include <algorithm>

#include <nlohmann/json.hpp>

#include "aimbot/input/InputConfig.h"

namespace aimbot::input {

    // -------- utils --------
    inline std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });
        return s;
    }

    inline DeviceType parseDevice(const std::string& s) {
        auto t = toLower(s);
        if (t == "keyboard") return DeviceType::Keyboard;
        if (t == "mouse")    return DeviceType::Mouse;
        if (t == "gamepad")  return DeviceType::Gamepad;
        return DeviceType::Keyboard;
    }

    inline TriggerType parseTrigger(const std::string& s) {
        auto t = toLower(s);
        if (t == "pressed")  return TriggerType::Pressed;
        if (t == "released") return TriggerType::Released;
        if (t == "down")     return TriggerType::Down;
        return TriggerType::Pressed;
    }

    inline BindingType parseBindingType(const std::string& s) {
        auto t = toLower(s);
        if (t == "button")  return BindingType::Button;
        if (t == "axis1d")  return BindingType::Axis1D;
        if (t == "axis2d")  return BindingType::Axis2D;
        if (t == "pointer") return BindingType::Pointer;
        return BindingType::Button;
    }

    // ---- key maps ----
    inline Key parseKey(const std::string& s) {
		using aimbot::input::Key;

		auto t = toLower(s);
		if (t.empty()) return Key::Unknown;

        // 1) Single character: letters / digits
        if (t.size() == 1) {
            const char c = t[0];
            if (c >= 'a' && c <= 'z') {
                return static_cast<Key>(static_cast<uint16_t>(Key::A) + (c - 'a'));
            }
            if (c >= '0' && c <= '9') {
                return static_cast<Key>(static_cast<uint16_t>(Key::D0) + (c - '0'));
            }
        }

        // 2) Function keys: f1..f12
        if (t[0] == 'f' && t.size() <= 3) {
            int n = 0;
            for (size_t i = 1; i < t.size(); ++i) {
                if (!std::isdigit(static_cast<unsigned char>(t[i]))) { n = 0; break; }
                n = n * 10 + (t[i] - '0');
            }
            if (n >= 1 && n <= 12) {
                return static_cast<Key>(static_cast<uint16_t>(Key::F1) + (n - 1));
            }
        }

        // 3) Named keys / aliases
        static const std::unordered_map<std::string, Key> kMap = {
            // Escape
            {"esc", Key::Escape}, {"escape", Key::Escape},

            // Enter / Tab / Space / Backspace
            {"enter", Key::Enter}, {"return", Key::Enter},
            {"tab", Key::Tab},
            {"space", Key::Space}, {"spacebar", Key::Space},
            {"backspace", Key::Backspace}, {"bs", Key::Backspace},

            // Insert/Delete
            {"insert", Key::Insert}, {"ins", Key::Insert},
            {"delete", Key::Delete}, {"del", Key::Delete},

            // Modifiers (explicit)
			// Shift/ Ctrl / Alt are mapped to left versions by default
            {"lshift", Key::LShift}, {"rshift", Key::RShift}, {"shift", Key::LShift},
            {"lctrl", Key::LCtrl},   {"rctrl", Key::RCtrl},   {"ctrl", Key::LCtrl}, {"control", Key::LCtrl},
            {"lalt", Key::LAlt},     {"ralt", Key::RAlt},     {"alt", Key::LAlt},

            {"capslock", Key::CapsLock}, {"caps", Key::CapsLock},

            // Arrows
            {"up", Key::Up}, {"arrowup", Key::Up},
            {"down", Key::Down}, {"arrowdown", Key::Down},
            {"left", Key::Left}, {"arrowleft", Key::Left},
            {"right", Key::Right}, {"arrowright", Key::Right},

            // Navigation
            {"home", Key::Home},
            {"end", Key::End},
            {"pageup", Key::PageUp}, {"pgup", Key::PageUp},
            {"pagedown", Key::PageDown}, {"pgdn", Key::PageDown},
        };

        auto it = kMap.find(t);
        if (it != kMap.end()) return it->second;

        return Key::Unknown;
    }

    inline MouseButton parseMouseButton(const std::string& s) {
        auto t = toLower(s);
        if (t == "lmb" || t == "left") return MouseButton::Left;
        if (t == "rmb" || t == "right") return MouseButton::Right;
        if (t == "mmb" || t == "middle") return MouseButton::Middle;
        if (t == "x1") return MouseButton::X1;
        if (t == "x2") return MouseButton::X2;
        return MouseButton::Left;
    }

    inline GamepadButton parseGamepadButton(const std::string& s) {
        auto t = toLower(s);
        if (t == "a") return GamepadButton::A;
        if (t == "b") return GamepadButton::B;
        if (t == "x") return GamepadButton::X;
        if (t == "y") return GamepadButton::Y;
        if (t == "lb") return GamepadButton::LB;
        if (t == "rb") return GamepadButton::RB;
        if (t == "back") return GamepadButton::Back;
        if (t == "start") return GamepadButton::Start;
        if (t == "dpadup") return GamepadButton::DPadUp;
        if (t == "dpaddown") return GamepadButton::DPadDown;
        if (t == "dpadleft") return GamepadButton::DPadLeft;
        if (t == "dpadright") return GamepadButton::DPadRight;
        if (t == "lstick") return GamepadButton::LStick;
        if (t == "rstick") return GamepadButton::RStick;
        return GamepadButton::A;
    }

    // -------- from_json --------
    inline void from_json(const nlohmann::json& j, Binding& b) {
        b.action = j.value("action", b.action);
        b.type = parseBindingType(j.value("type", "button"));

        // v1：只真正解析 button；其它类型先“识别但不做”
        if (b.type == BindingType::Button) {
            b.button.device = parseDevice(j.value("device", "keyboard"));
            b.button.trigger = parseTrigger(j.value("trigger", "pressed"));

            if (b.button.device == DeviceType::Keyboard) {
                b.button.key = parseKey(j.value("key", ""));
            }
            else if (b.button.device == DeviceType::Mouse) {
                b.button.mouse = parseMouseButton(j.value("button", "left"));
            }
            else {
                b.button.padIndex = j.value("pad", 0);
                b.button.padBtn = parseGamepadButton(j.value("button", "a"));
            }
        }
        else {
            // TODO: axis/pointer parsing when you implement them
            // You can still read deadzone/scale now without using them
            b.axis1d.deadzone = j.value("deadzone", b.axis1d.deadzone);
            b.axis1d.scale = j.value("scale", b.axis1d.scale);
            b.axis2d.deadzone = j.value("deadzone", b.axis2d.deadzone);
            b.axis2d.scale = j.value("scale", b.axis2d.scale);
            b.pointer.scale = j.value("scale", b.pointer.scale);
        }
    }

    inline void from_json(const nlohmann::json& j, InputConfig& c) {
        if (!j.is_object()) return;

        if (j.contains("bindings") && j["bindings"].is_array()) {
            c.bindings.clear();
            for (const auto& jb : j["bindings"]) {
                if (!jb.is_object()) continue;

                Binding b;
                from_json(jb, b);

                // 最小校验：action 不能为空；button 时必须能解析到有效 key/button
                if (b.action.empty()) continue;

                if (b.type == BindingType::Button) {
                    bool ok = true;
                    if (b.button.device == DeviceType::Keyboard) ok = (b.button.key != Key::Unknown);
                    // mouse/gamepad 默认都有一个枚举值，不额外判空
                    if (!ok) continue;
                }
                c.bindings.push_back(b);
            }
        }
    }

} // namespace aimbot::input
