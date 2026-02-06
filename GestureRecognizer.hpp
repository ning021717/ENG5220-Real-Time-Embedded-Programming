#ifndef GESTURE_RECOGNIZER_HPP
#define GESTURE_RECOGNIZER_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <deque>
#include <functional>
#include <string>
#include "SignDatabase.hpp"

class GestureRecognizer {
public:
    using RecognitionCallback = std::function<void(const std::string&)>;

    GestureRecognizer();

    // 核心处理函数
    void process(const cv::Mat& frame, const SignDatabase& db, RecognitionCallback callback);

private:
    std::deque<std::vector<float>> sequenceBuffer;

    // 修复：必须在这里声明这两个函数，.cpp 才能实现它们
    std::vector<float> extractFeatures(const cv::Mat& frame);
    float calculateDTW(const std::deque<std::vector<float>>& seq, const std::vector<float>& templ);
};

#endif