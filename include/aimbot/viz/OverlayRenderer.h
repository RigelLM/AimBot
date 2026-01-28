// viz/OverlayRenderer.h
// Lightweight visualization utilities for drawing detection results on frames.
//
// This module is intended for debugging and demo purposes:
// - Draw bounding boxes and centers for detected targets
// - Optionally annotate confidence / area / index
// - Overlay simple performance metrics (latency, FPS)
//
// All drawing is done in-place on a BGR OpenCV image (cv::Mat).

#pragma once
#include <vector>
#include <string>

#include <opencv2/opencv.hpp>

#include "aimbot/vision/Detection.h"
#include "aimbot/app/AppConfig.h"

class OverlayRenderer {
public:
    explicit OverlayRenderer(OverlayStyle style = {}) : style_(style) {}

    // Draw detection overlays on the given BGR image:
    // - bounding boxes
    // - center markers
    // - optional labels (confidence/area/index)
    //
    // bgr: image to draw on (modified in-place)
    // dets: detections to visualize
    void drawDetections(cv::Mat& bgr, const std::vector<Detection>& dets) const;

    // Draw a latency indicator (in milliseconds) at style_.latencyPos.
    void drawLatency(cv::Mat& bgr, double latencyMs) const;

    // Draw an FPS indicator at style_.fpsPos.
    void drawFPS(cv::Mat& bgr, double fps) const;

private:
    // Build a label string for a detection based on style_ flags.
    // index: detection index in the list (used if showIndex is enabled).
    std::string makeLabel(const Detection& d, int index) const;

private:
    // Rendering style (colors may be chosen in the .cpp implementation).
    OverlayStyle style_;
};