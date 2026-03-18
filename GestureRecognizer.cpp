#include "GestureRecognizer.hpp"
#include <iostream>

GestureRecognizer::GestureRecognizer(const std::string& modelPath) {
    knn = cv::ml::KNearest::load(modelPath);
    if (knn.empty()) {
        std::cerr << "[ERROR] Failed to load KNN model from: " << modelPath << std::endl;
    }
}

bool GestureRecognizer::isModelLoaded() const {
    return !knn.empty();
}

std::string GestureRecognizer::getLabelText(float label) const {
    int i = (int)label;
    // Dynamically map 0-25 back to characters A-Z
    if (i >= 0 && i < 26) {
        return std::string(1, 'A' + i);
    }
    return "Unknown";
}

std::string GestureRecognizer::predict(const cv::Mat& roi, cv::Mat& outMask) {
    if (roi.empty() || knn.empty()) {
        return "Error";
    }

    cv::Mat hsv, processingImg;

    // 1. Color Segmentation
    cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(H_MIN, S_MIN, V_MIN), cv::Scalar(H_MAX, S_MAX, V_MAX), outMask);

    // 2. Morphological Operations (Noise Reduction)
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::erode(outMask, outMask, kernel);
    cv::dilate(outMask, outMask, kernel);

    // 3. Hand Detection Check (Avoid processing empty backgrounds)
    if (cv::countNonZero(outMask) < 1000) {
        return "No Hand";
    }

    // 4. Feature Extraction & Formatting
    outMask.copyTo(processingImg);
    cv::resize(processingImg, processingImg, cv::Size(IMG_SIZE, IMG_SIZE));
    processingImg = processingImg.reshape(1, 1);
    processingImg.convertTo(processingImg, CV_32F);

    // 5. Machine Learning Prediction
    float result = knn->findNearest(processingImg, 5, cv::noArray());
    
    return getLabelText(result);
}
