// vision/ContourDetector.h
#pragma once
#include "Detection.h"
#include "HsvConfig.h"

#include <opencv2/opencv.hpp>
#include <vector>

class ContourDetector {
public:
    explicit ContourDetector(const HsvConfig& cfg) : cfg_(cfg) {}

    // 输入：二值mask
    // 输出：检测结果列表（每个包含bbox/center/area/confidence）
    std::vector<Detection> detect(const cv::Mat& mask);

private:
    float confidenceHeuristic(float area) const;

private:
    HsvConfig cfg_;
    std::vector<std::vector<cv::Point>> contours_;
    std::vector<cv::Vec4i> hierarchy_;
};
