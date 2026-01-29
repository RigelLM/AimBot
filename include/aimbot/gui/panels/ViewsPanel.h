#pragma once

#include <algorithm>

#include "aimbot/gui/panels/IPanel.h"
#include "aimbot/gui/AppModel.h"

namespace aimbot::gui::panels {

    static inline void DrawTexFit(void* id, int w, int h) {
        if (!id || w <= 0 || h <= 0) {
			ImGui::TextDisabled("No image");
            return;
        }
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float iw = (float)w, ih = (float)h;
        float s = std::min(avail.x / iw, avail.y / ih);
        if (s <= 0.0f) s = 1.0f;
        if (s > 1.0f) s = 1.0f;
        ImGui::Image((ImTextureID)id, ImVec2(iw * s, ih * s));
    }

    class ViewsPanel final : public PanelBase {
    public:
        const char* name() const override { return "Views"; }

    protected:
        enum class ViewMode : int {
            Screen = 0,
			Mask = 1
		};

        void drawContent(AppModel& m) override {
            ImGui::Text("Mode: %s", (mode_ == ViewMode::Screen) ? "Screen" : "Mask");
            ImGui::Separator();

			ImGui::BeginChild("ViewChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);

            //{
            //    const char* items[] = { "Screen", "Mask" };
            //    const char* current = items[(int)mode_];

            //    // 把控件放到右上角：用当前 child 的宽度减去控件宽度
            //    // 先估一个宽度（编辑器常见 120-160）
            //    const float comboW = 140.0f;
            //    ImVec2 p = ImGui::GetCursorPos();
            //    float xRight = ImGui::GetWindowContentRegionMax().x; // child 内 content max x
            //    ImGui::SetCursorPos(ImVec2(xRight - comboW, p.y));

            //    ImGui::SetNextItemWidth(comboW);
            //    if (ImGui::BeginCombo("##ViewMode", current)) {
            //        for (int i = 0; i < 2; ++i) {
            //            bool selected = ((int)mode_ == i);
            //            if (ImGui::Selectable(items[i], selected))
            //                mode_ = (ViewMode)i;
            //            if (selected) ImGui::SetItemDefaultFocus();
            //        }
            //        ImGui::EndCombo();
            //    }

            //    // 把 cursor 拉回去，不影响后续内容布局
            //    ImGui::SetCursorPos(p);
            //}

            {
                const float btnH = ImGui::GetFrameHeight();
                const float btnW = 70.0f;
                ImVec2 p = ImGui::GetCursorPos();
                float xRight = ImGui::GetWindowContentRegionMax().x;
                ImGui::SetCursorPos(ImVec2(xRight - (btnW * 2 + 6), p.y));

                bool screenOn = (mode_ == ViewMode::Screen);
                bool maskOn = (mode_ == ViewMode::Mask);

                if (screenOn) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                if (ImGui::Button("Screen", ImVec2(btnW, btnH))) mode_ = ViewMode::Screen;
                if (screenOn) ImGui::PopStyleColor();

                ImGui::SameLine(0, 6);

                if (maskOn) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                if (ImGui::Button("Mask", ImVec2(btnW, btnH))) mode_ = ViewMode::Mask;
                if (maskOn) ImGui::PopStyleColor();

                ImGui::SetCursorPos(p);
            }

            // 给右上角留一点空间，不然图片会顶到控件下面
            ImGui::Dummy(ImVec2(0, 16));

            // ---- 绘制当前视图 ----
            if (mode_ == ViewMode::Screen) {
                DrawTexFit(m.screenTex.imTextureID(), m.screenTex.width(), m.screenTex.height());
            }
            else {
                DrawTexFit(m.maskTex.imTextureID(), m.maskTex.width(), m.maskTex.height());
            }

            ImGui::EndChild();
        }
    private:
		ViewMode mode_ = ViewMode::Screen;
    };

} // namespace aimbot::gui::panels
