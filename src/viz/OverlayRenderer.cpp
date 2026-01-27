// OverlayRenderer.cpp
// Visualization helpers for drawing detection results and simple performance metrics.
//
// What this file does:
// - Build short text labels for detections (index / confidence / area) depending on style flags
// - Draw bounding boxes and center markers on a BGR frame
// - Draw latency and FPS text overlays
//
// Intended use:
// - Debugging / demo visualization while tuning HSV thresholds, contour filters, PID, etc.
// - Not performance-critical, but keeps allocations minimal and formatting simple.

#include "aimbot/viz/OverlayRenderer.h"

#include <cstdio>     // std::snprintf
#include <algorithm>  // std::max

// Compose the per-detection label text based on OverlayStyle toggles.
// Examples:
//   "#0 conf=0.73 area=5421"
//   "conf=0.73"
//   "area=5421"
// Returns an empty string if all label toggles are disabled.
std::string OverlayRenderer::makeLabel(const Detection& d, int index) const {
    char buf[128];

    // Combine fields as requested while keeping the string short.
    if (style_.showIndex && style_.showConfidence && style_.showArea) {
        std::snprintf(buf, sizeof(buf), "#%d conf=%.2f area=%.0f", index, d.confidence, d.area);
    }
    else if (style_.showIndex && style_.showConfidence) {
        std::snprintf(buf, sizeof(buf), "#%d conf=%.2f", index, d.confidence);
    }
    else if (style_.showIndex && style_.showArea) {
        std::snprintf(buf, sizeof(buf), "#%d area=%.0f", index, d.area);
    }
    else if (style_.showConfidence && style_.showArea) {
        std::snprintf(buf, sizeof(buf), "conf=%.2f area=%.0f", d.confidence, d.area);
    }
    else if (style_.showConfidence) {
        std::snprintf(buf, sizeof(buf), "conf=%.2f", d.confidence);
    }
    else if (style_.showArea) {
        std::snprintf(buf, sizeof(buf), "area=%.0f", d.area);
    }
    else if (style_.showIndex) {
        std::snprintf(buf, sizeof(buf), "#%d", index);
    }
    else {
        // Nothing to show.
        return {};
    }

    return std::string(buf);
}

// Draw bounding boxes, center markers, and optional labels for all detections.
// bgr: image to draw on (BGR cv::Mat), modified in-place
// dets: list of detections to visualize
void OverlayRenderer::drawDetections(cv::Mat& bgr, const std::vector<Detection>& dets) const {
    if (bgr.empty()) return;

    for (int i = 0; i < static_cast<int>(dets.size()); ++i) {
        const auto& d = dets[i];

        // Draw bounding box (green).
        cv::rectangle(bgr, d.bbox, cv::Scalar(0, 255, 0), style_.boxThickness);

        // Draw center marker (blue).
        cv::circle(bgr, d.center, style_.centerRadius, cv::Scalar(255, 0, 0), style_.centerThickness);

        // Draw label near the top-left of the bbox.
        // Place it slightly above the box to avoid covering content inside.
        std::string label = makeLabel(d, i);
        if (!label.empty()) {
            cv::Point org = d.bbox.tl() + cv::Point(0, -6);

            // Prevent the text from going above the image boundary.
            if (org.y < 12) org.y = d.bbox.y + 12;

            cv::putText(
                bgr, label, org,
                cv::FONT_HERSHEY_SIMPLEX, style_.fontScale,
                cv::Scalar(0, 255, 0), style_.fontThickness
            );
        }
    }
}

// Draw latency text (ms) at style_.latencyPos.
// latencyMs: measured time per frame or per pipeline iteration, in milliseconds.
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

// Draw FPS text at style_.fpsPos.
// fps: current frames per second estimate.
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
