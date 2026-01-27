// vision/HsvConfig.h
// HSV segmentation configuration used by HsvMasker (and often by ContourDetector).
//
// This struct defines:
//  - HSV threshold range [lower, upper] for color-based segmentation
//  - Minimum area filter to discard small noisy blobs
//  - Optional morphology parameters (open/close) to clean up the binary mask
//
// Notes:
//  - OpenCV uses HSV ranges: H in [0, 179], S in [0, 255], V in [0, 255] for 8-bit images.
//  - morphOpen / morphClose are typically interpreted as kernel sizes (e.g., 3, 5, 7).
//    If set to 0, the corresponding morphology step is skipped.

#pragma once

#include <opencv2/opencv.hpp>

struct HsvConfig {
    // Inclusive lower/upper HSV threshold (H, S, V).
    cv::Scalar lower{82, 199, 118};
    cv::Scalar upper{97, 255, 255};

    // Reject detections with area smaller than this value (in pixels).
    // Used to filter out small noise components after thresholding.
    int minArea = 1600;

    // Morphological opening kernel size (erode -> dilate) for removing small speckles.
    // 0 means disabled.
    int morphOpen = 0;
    // Morphological closing kernel size (dilate -> erode) for filling small holes.
    int morphClose = 0;
};
