#pragma once

#include "aimbot/gui/panels/IPanel.h"
#include "aimbot/gui/AppModel.h"

namespace aimbot::gui::panels {

    class ControlPanel final : public PanelBase {
    public:
        const char* name() const override { return "Control"; }

    protected:
        void drawContent(AppModel& m) override {
            ImGui::Text("Frame: %s", m.gotFrame ? "OK" : "NO");
            ImGui::Separator();

            if (ImGui::Button(m.assistEnabled ? "Disable (E)" : "Enable (Q)")) {
                m.setAssistEnabled(!m.assistEnabled);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Lock")) {
                m.resetLock();
            }

            ImGui::Separator();
            ImGui::Text("Latency: %.2f ms", m.latencyMs);
            ImGui::Text("Detections: %d", m.detCount);
            ImGui::Text("Hotkeys: Q enable, E disable, ESC quit");
        }
    };

} // namespace aimbot::gui::panels
