// ContourDetector.cpp
// Contour-based target detection on a binary mask.
//
// Input:
//   - mask: CV_8UC1 binary image (0 background, 255 foreground)
//
// Steps:
//   1) findContours with RETR_EXTERNAL + CHAIN_APPROX_SIMPLE
//      - RETR_EXTERNAL: only outer contours (ignore holes/inner contours)
//      - CHAIN_APPROX_SIMPLE: compresses horizontal/vertical segments to save memory
//   2) for each contour:
//      - skip tiny/degenerate contours
//      - compute bounding rect
//      - filter by minimum area (bbox area) using cfg_.minArea
//      - create a Detection with bbox, center, area, and a heuristic confidence
//
// Output:
//   - vector<Detection> (possibly empty)

#include "aimbot/vision/ContourDetector.h"

#include <algorithm> // std::min, std::max

std::vector<Detection> ContourDetector::detect(const cv::Mat& mask) {
    std::vector<Detection> out;
    out.clear();

    // Empty input => no detections.
    if (mask.empty()) return out;

    // Reuse internal buffers to avoid reallocations.
    contours_.clear();
    hierarchy_.clear();

    // Find connected components as contours.
    // Equivalent to the typical "find target" setup: external contours + simple approximation.
    cv::findContours(mask, contours_, hierarchy_, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    out.reserve(contours_.size());

    for (const auto& contour : contours_) {
        // Skip degenerate contours.
        if (contour.size() < 4) continue;

        // Compute axis-aligned bounding rectangle.
        cv::Rect r = cv::boundingRect(contour);

        // Filter by bbox area (fast heuristic).
        const int areaInt = r.width * r.height;
        if (areaInt < cfg_.minArea) continue;

        // Build detection record.
        Detection d;
        d.bbox = r;
        d.center = cv::Point2f(r.x + r.width * 0.5f, r.y + r.height * 0.5f);
        d.area = static_cast<float>(areaInt);

        // Heuristic confidence for ranking/overlay (not a probabilistic score).
        d.confidence = confidenceHeuristic(d.area);

        out.push_back(d);
    }

    return out;
}

float ContourDetector::confidenceHeuristic(float area) const {
    // Map area to a [0, 1] heuristic confidence score.
    //
    // Important:
    // - This is NOT a stronger detector; it is only a convenient score for visualization
    //   and basic ranking/filtering.
    // - You can replace this with better cues later:
    //     * mask fill ratio / solidity
    //     * aspect ratio constraints
    //     * contour area instead of bbox area
    //     * learned model score, etc.
    const float denom = static_cast<float>(cfg_.minArea) * 10.0f; // empirical scaling
    float c = area / denom;
    c = std::max(0.0f, std::min(1.0f, c));
    return c;
}
