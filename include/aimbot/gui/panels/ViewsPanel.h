#pragma once

#include <algorithm>

#include "aimbot/gui/panels/IPanel.h"
#include "aimbot/gui/AppModel.h"

namespace aimbot::gui::panels {

    static inline void DrawTex(void* id, int w, int h) {
        if (!id || w <= 0 || h <= 0) return;
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float iw = (float)w, ih = (float)h;
        float s = std::min(avail.x / iw, avail.y / ih);
        if (s > 1.0f) s = 1.0f;
        ImGui::Image((ImTextureID)id, ImVec2(iw * s, ih * s));
    }

    class ViewsPanel final : public PanelBase {
    public:
        const char* name() const override { return "Views"; }

    protected:
        void drawContent(AppModel& m) override {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float halfW = (avail.x - 10.0f) * 0.5f;

            ImGui::BeginChild("ScreenChild", ImVec2(halfW, avail.y), true);
            ImGui::TextUnformatted("Screen");
            DrawTex(m.screenTex.imTextureID(), m.screenTex.width(), m.screenTex.height());
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("MaskChild", ImVec2(halfW, avail.y), true);
            ImGui::TextUnformatted("Mask");
            DrawTex(m.maskTex.imTextureID(), m.maskTex.width(), m.maskTex.height());
            ImGui::EndChild();
        }
    };

} // namespace aimbot::gui::panels
