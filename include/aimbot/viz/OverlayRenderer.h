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
#include "../vision/Detection.h"

#include <vector>
#include <string>

#include <opencv2/opencv.hpp>

// Style configuration for overlay rendering.
// All values are in pixel units unless otherwise noted.
struct OverlayStyle {
    // Bounding box line thickness.
    int boxThickness = 2;

    // Radius of the center marker (circle) drawn at Detection.center.
    int centerRadius = 6;

    // Thickness of the center marker.
    // -1 means a filled circle in OpenCV.
    int centerThickness = -1; // -1 = filled

    // Text rendering parameters for cv::putText.
    double fontScale = 0.6;
    int fontThickness = 2;

    // On-screen positions for performance text overlays.
    cv::Point latencyPos{10, 30};
    cv::Point fpsPos{10, 60};

    // Label content toggles.
    bool showConfidence = true;
    bool showArea = false;
    bool showIndex = false;
};

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