// InputSystem.cpp
#include <cmath>

#include "aimbot/input/InputSystem.h"

namespace aimbot::input {

    InputSystem::InputSystem(InputConfig cfg, IInputBackend& backend)
        : cfg_(std::move(cfg)), backend_(&backend) {
        prevDown_.assign(cfg_.bindings.size(), false);
    }

    void InputSystem::setConfig(InputConfig cfg) {
        cfg_ = std::move(cfg);
        prevDown_.assign(cfg_.bindings.size(), false);
    }

    bool InputSystem::pollButtonDown(const ButtonBinding& b) const {
        switch (b.device) {
        case DeviceType::Keyboard: return backend_->keyDown(b.key);
        case DeviceType::Mouse:    return backend_->mouseDown(b.mouse);
        case DeviceType::Gamepad:  return backend_->gamepadDown(b.padIndex, b.padBtn);
        }
        return false;
    }

    void InputSystem::update() {
        backend_->pump();

        events_.clear();
        pressed_.clear();
        released_.clear();
        down_.clear();

        if (prevDown_.size() != cfg_.bindings.size())
            prevDown_.assign(cfg_.bindings.size(), false);

        for (size_t i = 0; i < cfg_.bindings.size(); ++i) {
            const auto& bind = cfg_.bindings[i];

            if (bind.type == BindingType::Button) {
                const bool now = pollButtonDown(bind.button);
                const bool prev = prevDown_[i];

                const bool isPressed = now && !prev;
                const bool isReleased = !now && prev;

                prevDown_[i] = now;

                // any binding can contribute to action down (OR)
                down_[bind.action] = down_[bind.action] || now;

                bool fire = false;
                switch (bind.button.trigger) {
                case TriggerType::Pressed:  fire = isPressed;  break;
                case TriggerType::Released: fire = isReleased; break;
                case TriggerType::Down:     fire = now;        break;
                }

                if (fire) {
                    if (bind.button.trigger == TriggerType::Pressed)  pressed_[bind.action] = true;
                    if (bind.button.trigger == TriggerType::Released) released_[bind.action] = true;
                    events_.push_back({ bind.action, bind.button.trigger });
                }
            }
            else if (bind.type == BindingType::Axis1D) {
                // TODO: implement mouse wheel / triggers
            }
            else if (bind.type == BindingType::Axis2D) {
                // TODO: implement sticks
            }
            else if (bind.type == BindingType::Pointer) {
                // TODO: implement raw pointer delta
            }
        }
    }

    bool InputSystem::pressed(const std::string& action) const {
        auto it = pressed_.find(action);
        return it != pressed_.end() && it->second;
    }
    bool InputSystem::released(const std::string& action) const {
        auto it = released_.find(action);
        return it != released_.end() && it->second;
    }
    bool InputSystem::down(const std::string& action) const {
        auto it = down_.find(action);
        return it != down_.end() && it->second;
    }

} // namespace
