// viz/OverlayRenderer.h
#pragma once
#include "../vision/Detection.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct OverlayStyle {
    int boxThickness = 2;
    int centerRadius = 6;
    int centerThickness = -1; // -1 = filled
    double fontScale = 0.6;
    int fontThickness = 2;

    // 文本位置
    cv::Point latencyPos{10, 30};
    cv::Point fpsPos{10, 60};

    // 是否显示内容
    bool showConfidence = true;
    bool showArea = false;
    bool showIndex = false;
};

class OverlayRenderer {
public:
    explicit OverlayRenderer(OverlayStyle style = {}) : style_(style) {}

    // 在 bgr 上绘制 dets（bbox + center + 文本）
    void drawDetections(cv::Mat& bgr, const std::vector<Detection>& dets) const;

    // 画延迟（毫秒）
    void drawLatency(cv::Mat& bgr, double latencyMs) const;

    // 画 FPS（可选）
    void drawFPS(cv::Mat& bgr, double fps) const;

private:
    std::string makeLabel(const Detection& d, int index) const;

private:
    OverlayStyle style_;
};