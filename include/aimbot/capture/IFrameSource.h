// capture/IFrameSource.h
// Abstract interface for a "frame source" that can provide BGR images as cv::Mat.
// Typical implementations:
// - Desktop capture (DXGI Desktop Duplication)
// - Camera capture
// - Video file reader
// Consumers can depend on this interface to stay decoupled from capture backend details.
#pragma once

#include <opencv2/opencv.hpp>

class IFrameSource {
public:
    virtual ~IFrameSource() = default;

    // Acquire the latest frame and write it to outBgr (expected CV_8UC3, BGR).
    // Returns true if a frame was successfully grabbed.
    virtual bool grab(cv::Mat& outBgr) = 0;
};
