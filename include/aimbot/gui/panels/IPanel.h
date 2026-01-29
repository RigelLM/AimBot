#pragma once

#include <imgui.h>

namespace aimbot::gui { class AppModel; }

namespace aimbot::gui::panels {

    // A: interface
    struct IPanel {
        virtual ~IPanel() = default;
        virtual const char* name() const = 0;
        virtual void draw(AppModel& m) = 0;
    };

    // B: base class: handles Begin/End + open flag
    class PanelBase : public IPanel {
    public:
        const char* name() const override = 0;

        void draw(AppModel& m) final {
            if (!open_) return;
            if (ImGui::Begin(name(), &open_)) {
                drawContent(m);
            }
            ImGui::End();
        }

        bool isOpen() const { return open_; }
        void setOpen(bool v) { open_ = v; }

    protected:
        virtual void drawContent(AppModel& m) = 0;
        bool open_ = true;
    };

} // namespace aimbot::gui::panels
