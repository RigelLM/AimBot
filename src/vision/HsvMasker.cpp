#include "aimbot/vision/HsvMasker.h"

void HsvMasker::run(const cv::Mat& bgr, cv::Mat& outMask) {
    if (bgr.empty()) {
        outMask.release();
        return;
    }

    // 你的 capture 输出是 BGR，所以这里必须是 BGR2HSV（和你原来一致）
    cv::cvtColor(bgr, hsv_, cv::COLOR_BGR2HSV);
    cv::inRange(hsv_, cfg_.lower, cfg_.upper, outMask);

    // Step2：为了“等价迁移”，这里默认不做额外处理
    // 但你以后 Step3/可配置 pipeline 可以打开这些参数
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
