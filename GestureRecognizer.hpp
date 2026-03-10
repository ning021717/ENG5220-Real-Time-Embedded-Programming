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

   // Orchestrates feature extraction, sequence buffering, and gesture matching
    void process(const cv::Mat& frame, const SignDatabase& db, RecognitionCallback callback);

private:
    // Circular buffer to store sequential feature vectors (max 30 frames)
    std::deque<std::vector<float>> sequenceBuffer;

    // Fix: These two functions MUST be declared here to be implemented in the .cpp file
    // (Class member functions need declaration in header before implementation in source file)
    std::vector<float> extractFeatures(const cv::Mat& frame);
    float calculateDTW(const std::deque<std::vector<float>>& seq, const std::vector<float>& templ);
};


#endif
