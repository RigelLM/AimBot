#include "aimbot/viz/OverlayRenderer.h"
#include <cstdio>     // std::snprintf
#include <algorithm>  // std::max

std::string OverlayRenderer::makeLabel(const Detection& d, int index) const {
    char buf[128];

    // 按需组合信息（尽量别太长）
    // 例：#0 conf=0.73 area=5421
    if (style_.showIndex && style_.showConfidence && style_.showArea) {
        std::snprintf(buf, sizeof(buf), "#%d conf=%.2f area=%.0f", index, d.confidence, d.area);
    } else if (style_.showIndex && style_.showConfidence) {
        std::snprintf(buf, sizeof(buf), "#%d conf=%.2f", index, d.confidence);
    } else if (style_.showIndex && style_.showArea) {
        std::snprintf(buf, sizeof(buf), "#%d area=%.0f", index, d.area);
    } else if (style_.showConfidence && style_.showArea) {
        std::snprintf(buf, sizeof(buf), "conf=%.2f area=%.0f", d.confidence, d.area);
    } else if (style_.showConfidence) {
        std::snprintf(buf, sizeof(buf), "conf=%.2f", d.confidence);
    } else if (style_.showArea) {
        std::snprintf(buf, sizeof(buf), "area=%.0f", d.area);
    } else if (style_.showIndex) {
        std::snprintf(buf, sizeof(buf), "#%d", index);
    } else {
        return {};
    }

    return std::string(buf);
}

void OverlayRenderer::drawDetections(cv::Mat& bgr, const std::vector<Detection>& dets) const {
    if (bgr.empty()) return;

    for (int i = 0; i < static_cast<int>(dets.size()); ++i) {
        const auto& d = dets[i];

        // bbox
        cv::rectangle(bgr, d.bbox, cv::Scalar(0, 255, 0), style_.boxThickness);

        // center
        cv::circle(bgr, d.center, style_.centerRadius, cv::Scalar(255, 0, 0), style_.centerThickness);

        // label（放在 bbox 左上角上方，避免挡住框）
        std::string label = makeLabel(d, i);
        if (!label.empty()) {
            cv::Point org = d.bbox.tl() + cv::Point(0, -6);
            if (org.y < 12) org.y = d.bbox.y + 12; // 防止文字跑出画面顶部

            cv::putText(
                bgr, label, org,
                cv::FONT_HERSHEY_SIMPLEX, style_.fontScale,
                cv::Scalar(0, 255, 0), style_.fontThickness
            );
        }
    }
}

void OverlayRenderer::drawLatency(cv::Mat& bgr, double latencyMs) const {
    if (bgr.empty()) return;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Latency: %.1f ms", latencyMs);

    cv::putText(
        bgr, buf, style_.latencyPos,
        cv::FONT_HERSHEY_SIMPLEX, 1.0,
        cv::Scalar(0, 255, 0), 2
    );
}

void OverlayRenderer::drawFPS(cv::Mat& bgr, double fps) const {
    if (bgr.empty()) return;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "FPS: %.1f", fps);

    cv::putText(
        bgr, buf, style_.fpsPos,
        cv::FONT_HERSHEY_SIMPLEX, 1.0,
        cv::Scalar(0, 255, 0), 2
    );
}
