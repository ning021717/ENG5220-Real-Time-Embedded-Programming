#include "GestureRecognizer.hpp"

GestureRecognizer::GestureRecognizer() {
    lower_skin = cv::Scalar(5, 30, 80);
    upper_skin = cv::Scalar(30, 255, 255);
}

double GestureRecognizer::calculateDistance(cv::Point p1, cv::Point p2) {
    return std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
}

void GestureRecognizer::processFrame(const cv::Mat& input, GestureCallback callback) {
    if (input.empty()) return;

    cv::Mat hsv, mask;
    cv::cvtColor(input, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, lower_skin, upper_skin, mask);

    // 降噪
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (!contours.empty()) {
        // 找到最大轮廓
        auto it = std::max_element(contours.begin(), contours.end(),
            [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                return cv::contourArea(a) < cv::contourArea(b);
            });

        if (cv::contourArea(*it) < 2000) return;

        std::vector<int> hullIdx;
        std::vector<cv::Vec4i> defects;
        cv::convexHull(*it, hullIdx, false);
        if (hullIdx.size() > 3) {
            cv::convexityDefects(*it, hullIdx, defects);
        }

        int fingers = 0;
        for (const auto& d : defects) {
            float depth = d[3] / 256.0;
            if (depth > 30) fingers++;
        }

        // 映射逻辑：手指+1（基础手掌）
        int finalCount = std::min(std::max(fingers + 1, 1), 5);

        // 执行回调：将手指数量传给翻译模块
        if (callback) callback(finalCount);

        // 调试用
        cv::imshow("Mask", mask);
    }
}//
// Created by 金扫帚奖最佳演员 on 2026/2/6.
//