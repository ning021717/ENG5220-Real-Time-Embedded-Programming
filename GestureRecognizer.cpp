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

    cv::Mat ycrcb, processingImg;

    // 1. YCrCb Skin Segmentation
    // YCrCb separates luminance (Y) from chrominance (Cr, Cb), making skin
    // detection robust to lighting changes — bright or dim light only shifts Y,
    // while skin Cr/Cb values stay stable across illumination conditions.
    cv::cvtColor(roi, ycrcb, cv::COLOR_BGR2YCrCb);
    cv::inRange(ycrcb,
                cv::Scalar(0,    CR_MIN, CB_MIN),
                cv::Scalar(255,  CR_MAX, CB_MAX),
                outMask);

    // 2. Morphological Closing (dilate → erode) to fill hollow palm regions,
    //    followed by a second closing with a larger kernel for stubborn gaps.
    cv::Mat kernel5 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::Mat kernel9 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
    cv::dilate(outMask, outMask, kernel5);
    cv::erode(outMask,  outMask, kernel5);
    cv::dilate(outMask, outMask, kernel9);
    cv::erode(outMask,  outMask, kernel9);

    // 3. Hand Detection Check
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
