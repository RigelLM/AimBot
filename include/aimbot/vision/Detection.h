// vision/Detection.h
#pragma once
#include <opencv2/opencv.hpp>

struct Detection {
    cv::Rect bbox;
    cv::Point2f center;
    float confidence = 0.0f;   // 0~1，先用启发式也行
    float area = 0.0f;         // bbox 或 contour area
};
