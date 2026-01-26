// vision/HsvMasker.h
#pragma once
#include "HsvConfig.h"
#include <opencv2/opencv.hpp>

class HsvMasker {
public:
    explicit HsvMasker(HsvConfig cfg) : cfg_(cfg) {}

    // 输入 BGR，输出 mask（CV_8UC1）
    void run(const cv::Mat& bgr, cv::Mat& outMask);

    const HsvConfig& config() const { return cfg_; }
    HsvConfig& config() { return cfg_; }
private:
    HsvConfig cfg_;
    cv::Mat hsv_;
};
