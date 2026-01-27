// vision/Detection.h
// Detection result structure.
//
// Represents one detected candidate target region.
// Fields are intentionally lightweight so that multiple detections can be returned
// per frame and scored/sorted by the caller.
#pragma once
#include <opencv2/opencv.hpp>

struct Detection {
    // Axis-aligned bounding box in image coordinates.
    cv::Rect bbox;

    // Convenience: center of the bounding box (or contour centroid),
    // in float precision for smoother downstream control.
    cv::Point2f center;

    // A heuristic "confidence" score (not a learned probability).
    // Used for ranking/filtering detections.
    float confidence = 0.0f;

    // Area of the detection region (pixels), typically bbox area or contour area.
    float area = 0.0f;
};
