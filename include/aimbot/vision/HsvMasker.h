// vision/HsvMasker.h
// HSV-based color segmentation module.
//
// Purpose:
// - Convert an input BGR image to HSV color space
// - Threshold HSV using a configurable lower/upper range
// - Optionally apply simple morphology (open/close) depending on HsvConfig
// - Output a binary mask (CV_8UC1) for downstream contour/target detection
//
// Typical usage:
//   HsvMasker masker(cfg);
//   cv::Mat mask;
//   masker.run(bgrFrame, mask);
#pragma once

#include <opencv2/opencv.hpp>

#include "aimbot/app/AppConfig.h"

class HsvMasker {
public:
    // Construct with a copy of configuration (thresholds + morphology parameters).
    explicit HsvMasker(HsvConfig cfg) : cfg_(cfg) {}

    // Run masking on an input BGR image.
    // bgr: input image in BGR format (usually CV_8UC3)
    // outMask: output binary mask (CV_8UC1), white = pixels within HSV range
    void run(const cv::Mat& bgr, cv::Mat& outMask);

    // Access configuration (const and mutable).
    const HsvConfig& config() const { return cfg_; }
    HsvConfig& config() { return cfg_; }
private:
    // Current masking configuration.
    HsvConfig cfg_;

    // Internal buffer to avoid reallocations (BGR -> HSV).
    cv::Mat hsv_;
};
