#pragma once
#include <opencv2/core.hpp>
#include "aimbot/vision/HsvConfig.h"

struct CaptureConfig {
    int offset_x = 0;
    int offset_y = 0;
};

struct LockConfig {
    float match_radius = 60.0f;
    int lost_frames_to_unlock = 5;
};

struct AppConfig {
    CaptureConfig capture;
    HsvConfig hsv;
    LockConfig lock;
};
