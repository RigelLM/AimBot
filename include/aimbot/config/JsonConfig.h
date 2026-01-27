// config/JsonConfig.h
#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

inline cv::Scalar scalar3_from_json(const nlohmann::json& a, const cv::Scalar& def) {
    if (!a.is_array() || a.size() < 3) return def;
    return cv::Scalar(a[0].get<double>(), a[1].get<double>(), a[2].get<double>());
}

inline cv::Point point2_from_json(const nlohmann::json& a, const cv::Point& def) {
    if (!a.is_array() || a.size() < 2) return def;
    return cv::Point(a[0].get<int>(), a[1].get<int>());
}
