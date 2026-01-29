// input/InputSystem.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "aimbot/input/InputConfig.h"
#include "aimbot/input/IInputBackend.h"

namespace aimbot::input {

    struct ActionEvent {
        std::string action;
        TriggerType trigger;
    };

    class InputSystem {
    public:
        InputSystem(InputConfig cfg, IInputBackend& backend);

        void setConfig(InputConfig cfg);

        void update(); // 每帧调用

        bool pressed(const std::string& action) const;
        bool released(const std::string& action) const;
        bool down(const std::string& action) const;

        const std::vector<ActionEvent>& events() const { return events_; }

    private:
        bool pollButtonDown(const ButtonBinding& b) const;

    private:
        InputConfig cfg_;
        IInputBackend* backend_{};

        // per-action state
        std::unordered_map<std::string, bool> down_;
        std::unordered_map<std::string, bool> pressed_;
        std::unordered_map<std::string, bool> released_;
        std::vector<ActionEvent> events_;

        // per-binding prev state (edge detect)
        std::vector<bool> prevDown_; // same length as cfg_.bindings
    };

} // namespace
