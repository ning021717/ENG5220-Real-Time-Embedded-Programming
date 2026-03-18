#ifndef GESTURE_RECOGNIZER_HPP
#define GESTURE_RECOGNIZER_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <string>

class GestureRecognizer {
public:
    // Constructor: Loads the KNN model
    GestureRecognizer(const std::string& modelPath);
    ~GestureRecognizer() = default;

    // Check if model is successfully loaded
    bool isModelLoaded() const;

    // Core function: Takes the ROI, returns the predicted text and outputs the binary mask
    std::string predict(const cv::Mat& roi, cv::Mat& outMask);

    // Public variables for GUI Trackbar bindings
    int H_MIN = 0;
    int H_MAX = 20;
    int S_MIN = 30;
    int S_MAX = 255;
    int V_MIN = 30;
    int V_MAX = 255;

private:
    cv::Ptr<cv::ml::KNearest> knn; // Encapsulated Machine Learning Engine
    const int IMG_SIZE = 50;       // Must match the training size

    // Internal helper to convert numeric prediction to A-Z
    std::string getLabelText(float label) const;
};

#endif // GESTURE_RECOGNIZER_HPP
