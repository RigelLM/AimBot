// vision/HsvConfig.h
#pragma once
#include <opencv2/opencv.hpp>

struct HsvConfig {
    cv::Scalar lower{82, 199, 118};
    cv::Scalar upper{97, 255, 255};

    int minArea = 1600; // bbox面积过滤阈值

    // 可选：形态学去噪
    int morphOpen = 0;   // 0 表示不做
    int morphClose = 0;
};
