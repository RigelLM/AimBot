#pragma once

#include <algorithm>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "aimbot/gui/panels/IPanel.h"
#include "aimbot/gui/AppModel.h"

namespace aimbot::gui::panels {

    // ---- helpers ----
    static inline bool DragDouble(const char* label, double* v, double speed, double v_min, double v_max, const char* fmt = "%.3f") {
        float vs = (float)speed;
        return ImGui::DragScalar(label, ImGuiDataType_Double, v, vs, &v_min, &v_max, fmt);
    }

    static inline bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* fmt = "%.3f") {
        return ImGui::SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, fmt);
    }

    static inline bool EditHsvScalar3(const char* label, cv::Scalar& s, int min0, int max0, int min1, int max1, int min2, int max2)
    {
        int v[3] = { (int)s[0], (int)s[1], (int)s[2] };
        bool changed = ImGui::SliderInt3(label, v, 0, 255);
        // But HSV has different ranges: H[0,179], S/V[0,255]
        // So we clamp manually to desired ranges.
        v[0] = std::clamp(v[0], min0, max0);
        v[1] = std::clamp(v[1], min1, max1);
        v[2] = std::clamp(v[2], min2, max2);

        if (changed) {
            s = cv::Scalar(v[0], v[1], v[2]);
        }
        return changed;
    }

    static inline bool EditPoint2i(const char* label, cv::Point& p)
    {
        int v[2] = { p.x, p.y };
        bool changed = ImGui::DragInt2(label, v, 1.0f, -10000, 10000);
        if (changed) {
            p.x = v[0];
            p.y = v[1];
        }
        return changed;
    }

    static inline bool EditPIDAxis(const char* label, PIDAxis& a)
    {
        bool changed = false;
        if (ImGui::TreeNode(label))
        {
            changed |= DragDouble("kp", &a.kp, 0.001, 0.0, 10.0, "%.4f");
            changed |= DragDouble("ki", &a.ki, 0.001, 0.0, 10.0, "%.4f");
            changed |= DragDouble("kd", &a.kd, 0.001, 0.0, 10.0, "%.4f");

            ImGui::SeparatorText("Output clamp (per tick)");
            changed |= DragDouble("outMin", &a.outMin, 0.5, -5000.0, 0.0, "%.2f");
            changed |= DragDouble("outMax", &a.outMax, 0.5, 0.0, 5000.0, "%.2f");

            ImGui::SeparatorText("Integral clamp");
            changed |= DragDouble("iMin", &a.iMin, 1.0, -1e7, 0.0, "%.1f");
            changed |= DragDouble("iMax", &a.iMax, 1.0, 0.0, 1e7, "%.1f");

            ImGui::SeparatorText("Derivative filter");
            changed |= SliderDouble("dAlpha", &a.dAlpha, 0.0, 0.999, "%.3f");

            ImGui::TreePop();
        }
        return changed;
    }

    static inline ImVec4 HsvOpenCvToImVec4(int H, int S, int V)
    {
        H = std::clamp(H, 0, 179);
        S = std::clamp(S, 0, 255);
        V = std::clamp(V, 0, 255);

        // OpenCV uses BGR in cv::Mat by default
        cv::Mat3b hsv(1, 1, cv::Vec3b((uchar)H, (uchar)S, (uchar)V));
        cv::Mat3b bgr;
        cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);

        cv::Vec3b c = bgr(0, 0);
        float r = c[2] / 255.0f;
        float g = c[1] / 255.0f;
        float b = c[0] / 255.0f;
        return ImVec4(r, g, b, 1.0f);
    }

    static inline void DrawColorSwatch(const char* label, const ImVec4& col, const ImVec2& size = ImVec2(22, 22))
    {
        ImGui::ColorButton(label, col,
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
            size);
    }

    static inline void DrawHsvRangePreview(const cv::Scalar& lower, const cv::Scalar& upper)
    {
        int lh = (int)lower[0], ls = (int)lower[1], lv = (int)lower[2];
        int uh = (int)upper[0], us = (int)upper[1], uv = (int)upper[2];

        bool wrap = (lh > uh);

        // 1) threshold preview (uses actual S/V)
        ImVec4 colLower = HsvOpenCvToImVec4(lh, ls, lv);
        ImVec4 colUpper = HsvOpenCvToImVec4(uh, us, uv);

        // 2) hue-only preview (force vivid)
        ImVec4 hueLower = HsvOpenCvToImVec4(lh, 255, 255);
        ImVec4 hueUpper = HsvOpenCvToImVec4(uh, 255, 255);

        ImGui::SeparatorText("HSV Preview");

        ImGui::Text("Hue range: %d -> %d %s", lh, uh, wrap ? "(wrap)" : "");
        ImGui::SameLine();
        ImGui::TextDisabled("(OpenCV H:0..179)");

        ImGui::TextUnformatted("Hue-only:");
        ImGui::SameLine();
        DrawColorSwatch("##hueL", hueLower);
        ImGui::SameLine();
        ImGui::TextUnformatted("...");
        ImGui::SameLine();
        DrawColorSwatch("##hueU", hueUpper);

        ImGui::TextUnformatted("Threshold (actual S/V):");
        ImGui::SameLine();
        DrawColorSwatch("##thrL", colLower);
        ImGui::SameLine();
        ImGui::TextUnformatted("...");
        ImGui::SameLine();
        DrawColorSwatch("##thrU", colUpper);

        if (wrap) {
            ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "Wrap-around active: range is [0..%d] U [%d..179]", uh, lh);
        }
    }

    static inline bool DrawConfigUI(AppConfig& uiCfg)
    {
        bool changed = false;

        // ---------------- Capture ----------------
        if (ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragInt("offset_x", &uiCfg.capture.offset_x, 1.0f, -10000, 10000);
            changed |= ImGui::DragInt("offset_y", &uiCfg.capture.offset_y, 1.0f, -10000, 10000);
            ImGui::TextUnformatted("ROI -> Screen mapping: screen = roi + offset");
        }

        // ---------------- HSV ----------------
        if (ImGui::CollapsingHeader("HSV Mask", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted("OpenCV HSV ranges: H[0..179], S/V[0..255]");

            // Lower / Upper
            changed |= EditHsvScalar3("lower (H,S,V)", uiCfg.hsv.lower, 0, 179, 0, 255, 0, 255);
            changed |= EditHsvScalar3("upper (H,S,V)", uiCfg.hsv.upper, 0, 179, 0, 255, 0, 255);

			DrawHsvRangePreview(uiCfg.hsv.lower, uiCfg.hsv.upper);

            ImGui::Separator();
            changed |= ImGui::DragInt("minArea", &uiCfg.hsv.minArea, 10.0f, 0, 1000000);
            changed |= ImGui::DragInt("morphOpen", &uiCfg.hsv.morphOpen, 1.0f, 0, 99);
            changed |= ImGui::DragInt("morphClose", &uiCfg.hsv.morphClose, 1.0f, 0, 99);
            ImGui::TextUnformatted("morphOpen/Close = kernel size (0 disables)");
        }

        // ---------------- Lock ----------------
        if (ImGui::CollapsingHeader("Target Lock", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragFloat("match_radius", &uiCfg.lock.match_radius, 0.5f, 0.0f, 2000.0f, "%.1f");
            changed |= ImGui::DragInt("lost_frames_to_unlock", &uiCfg.lock.lost_frames_to_unlock, 1.0f, 0, 300);
        }

        // ---------------- Cursor Assist ----------------
        if (ImGui::CollapsingHeader("Cursor Assist", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragInt("tick_hz", &uiCfg.cursor.tick_hz, 1.0f, 1, 2000);
            changed |= ImGui::DragInt("deadzone_px", &uiCfg.cursor.deadzone_px, 1.0f, 0, 100);

            ImGui::SeparatorText("Auto click (dwell)");
            changed |= ImGui::Checkbox("auto_click", &uiCfg.cursor.auto_click);
            changed |= DragDouble("click_dist_px", &uiCfg.cursor.click_dist_px, 0.5, 0.0, 500.0, "%.1f");
            changed |= ImGui::DragInt("dwell_ms", &uiCfg.cursor.dwell_ms, 1.0f, 0, 10000);
            changed |= ImGui::DragInt("cooldown_ms", &uiCfg.cursor.cooldown_ms, 1.0f, 0, 60000);

            ImGui::SeparatorText("Worker loop");
            changed |= ImGui::DragInt("min_sleep_us", &uiCfg.cursor.min_sleep_us, 10.0f, 0, 2000000);

            ImGui::SeparatorText("PID");
            changed |= EditPIDAxis("PID X", uiCfg.cursor.pid.x);
            changed |= EditPIDAxis("PID Y", uiCfg.cursor.pid.y);

            ImGui::TextUnformatted("NOTE: Apply should reset PID internal state (integral/prevError).");
        }

        // ---------------- Overlay ----------------
        if (ImGui::CollapsingHeader("Overlay Style", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragInt("boxThickness", &uiCfg.overlay.boxThickness, 1.0f, 1, 20);
            changed |= ImGui::DragInt("centerRadius", &uiCfg.overlay.centerRadius, 1.0f, 0, 50);
            changed |= ImGui::DragInt("centerThickness", &uiCfg.overlay.centerThickness, 1.0f, -1, 20);

            changed |= DragDouble("fontScale", &uiCfg.overlay.fontScale, 0.01f, 0.1f, 3.0f, "%.2f");
            changed |= ImGui::DragInt("fontThickness", &uiCfg.overlay.fontThickness, 1.0f, 1, 10);

            changed |= EditPoint2i("latencyPos (x,y)", uiCfg.overlay.latencyPos);
            changed |= EditPoint2i("fpsPos (x,y)", uiCfg.overlay.fpsPos);

            ImGui::SeparatorText("Labels");
            changed |= ImGui::Checkbox("showConfidence", &uiCfg.overlay.showConfidence);
            changed |= ImGui::Checkbox("showArea", &uiCfg.overlay.showArea);
            changed |= ImGui::Checkbox("showIndex", &uiCfg.overlay.showIndex);
        }

        return changed;
    }

    class ConfigPanel final : public PanelBase {
    public:
        const char* name() const override { return "Config"; }

    protected:
        void drawContent(AppModel& m) override {
            if (ImGui::Button("Apply")) m.pendingApply = true;
            ImGui::SameLine();
            if (ImGui::Button("Save"))  m.pendingSave = true;
            ImGui::SameLine();
            if (ImGui::Button("Revert")) { m.uiCfg = m.cfg; m.uiDirty = false; }

            ImGui::SameLine();
            ImGui::Text("Dirty: %s", m.uiDirty ? "YES" : "NO");

            ImGui::Separator();
            m.uiDirty |= DrawConfigUI(m.uiCfg);

            // Apply/Save 不在 Panel 内实现：只发“意图”，由 m.processPendingActions() 统一处理
            // TODO(apply contract):
            // 1) cfg = uiCfg; uiDirty=false;
            // 2) apply to modules: masker/detector/overlay/targetLock/controller
            // 3) reset runtime states: resetLock(), reset pid state ...
            //
            // TODO(save contract):
            // SaveAppConfig("config/app.json", uiCfg, &err)
        }
    };

} // namespace aimbot::gui::panels
