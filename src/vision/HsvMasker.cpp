// HsvMasker.cpp
// HSV-based color segmentation.
//
// Input:
//   - bgr: CV_8UC3 image in BGR color space (as produced by the capture module)
//
// Output:
//   - outMask: CV_8UC1 binary mask where foreground pixels fall within [cfg_.lower, cfg_.upper]
//
// Steps:
//   1) Convert BGR -> HSV
//   2) Threshold HSV range using inRange()
//   3) Optionally apply morphology (open/close) for noise removal / hole filling

#include "aimbot/vision/HsvMasker.h"

void HsvMasker::run(const cv::Mat& bgr, cv::Mat& outMask) {
    // Empty input => clear output.
    if (bgr.empty()) {
        outMask.release();
        return;
    }

    // Capture outputs BGR, so convert with COLOR_BGR2HSV (not RGB2HSV).
    cv::cvtColor(bgr, hsv_, cv::COLOR_BGR2HSV);

    // Threshold HSV range -> binary mask.
    cv::inRange(hsv_, cfg_.lower, cfg_.upper, outMask);

    // Optional cleanup:
    // - Opening removes small speckles (erode then dilate)
    // - Closing fills small holes (dilate then erode)
    // Set morphOpen/morphClose to 0 to disable (default behavior).
    if (cfg_.morphOpen > 0) {
        auto k = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, cv::Size(cfg_.morphOpen, cfg_.morphOpen));
        cv::morphologyEx(outMask, outMask, cv::MORPH_OPEN, k);
    }
    if (cfg_.morphClose > 0) {
        auto k = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, cv::Size(cfg_.morphClose, cfg_.morphClose));
        cv::morphologyEx(outMask, outMask, cv::MORPH_CLOSE, k);
    }
}
