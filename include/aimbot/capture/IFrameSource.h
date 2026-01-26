// capture/IFrameSource.h
#pragma once
#include <opencv2/opencv.hpp>

class IFrameSource {
public:
    virtual ~IFrameSource() = default;
    virtual bool grab(cv::Mat& outBgr) = 0; // 成功返回 true
};
