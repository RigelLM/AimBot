// vision/ContourDetector.h
// Contour-based detector operating on a binary mask.
//
// Pipeline idea:
// - Input: a binary mask (e.g., produced by HsvMasker), where foreground pixels
//   correspond to the target color/region.
// - Find connected components via contours (cv::findContours).
// - For each contour:
//     * compute area (contour area or bbox area)
//     * filter by minimum area threshold (cfg_.minArea, etc.)
//     * compute bbox + center
//     * compute a heuristic confidence based on area (and potentially other cues)
// - Output: a list of Detection candidates.
//
// Notes:
// - This is a classic, fast CV approach (no ML), suitable for real-time tracking.

#pragma once

#include "Detection.h"
#include "HsvConfig.h"

#include <vector>

#include <opencv2/opencv.hpp>

class ContourDetector {
public:
    // The detector uses configuration primarily for area thresholds and related filters.
    explicit ContourDetector(const HsvConfig& cfg) : cfg_(cfg) {}

    // Detect candidate targets from a binary mask.
    // mask: CV_8UC1 binary image, foreground = 255, background = 0
    // returns: list of detections (possibly empty)
    std::vector<Detection> detect(const cv::Mat& mask);

private:
    // Map an area measurement to a simple confidence score.
    // This is a heuristic used for ranking; not a calibrated probability.
    float confidenceHeuristic(float area) const;

private:
    // Configuration snapshot (thresholds, min area, etc.).
    HsvConfig cfg_;

    // Internal buffers reused across frames to reduce allocations.
    std::vector<std::vector<cv::Point>> contours_;
    std::vector<cv::Vec4i> hierarchy_;
};
