#include "aimbot/vision/ContourDetector.h"
#include <algorithm> // std::min, std::max

std::vector<Detection> ContourDetector::detect(const cv::Mat& mask) {
    std::vector<Detection> out;
    out.clear();

    if (mask.empty()) return out;

    contours_.clear();
    hierarchy_.clear();

    // 等价于你 Tool::findtarget 的参数：RETR_EXTERNAL + CHAIN_APPROX_SIMPLE
    cv::findContours(mask, contours_, hierarchy_, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    out.reserve(contours_.size());

    for (const auto& contour : contours_) {
        // size < 4 跳过
        if (contour.size() < 4) continue;

        // boundingRect
        cv::Rect r = cv::boundingRect(contour);

        // r.width * r.height < 1600 跳过
        const int areaInt = r.width * r.height;
        if (areaInt < cfg_.minArea) continue;

        Detection d;
        d.bbox = r;
        d.center = cv::Point2f(r.x + r.width * 0.5f, r.y + r.height * 0.5f);
        d.area = static_cast<float>(areaInt);
        d.confidence = confidenceHeuristic(d.area);

        out.push_back(d);
    }

    return out;
}

float ContourDetector::confidenceHeuristic(float area) const {
    // 占位：把面积映射到 0~1
    // 说明：这不是“更强检测”，只是为了你后续 Overlay 能显示一个 conf 字段。
    // 你随时可以换成：mask覆盖率、solidity、长宽比、或模型输出分数等。
    const float denom = static_cast<float>(cfg_.minArea) * 10.0f; // 经验缩放
    float c = area / denom;
    c = std::max(0.0f, std::min(1.0f, c));
    return c;
}
