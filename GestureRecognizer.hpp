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

    // Public variables for GUI Trackbar bindings (YCrCb skin segmentation)
    // Y: luminance (ignored for skin), Cr: red-diff, Cb: blue-diff
    // Typical skin range under varied lighting: Cr [133,173], Cb [77,127]
    int CR_MIN = 133;
    int CR_MAX = 173;
    int CB_MIN = 77;
    int CB_MAX = 127;

private:
    cv::Ptr<cv::ml::KNearest> knn; // Encapsulated Machine Learning Engine
    const int IMG_SIZE = 50;       // Must match the training size

    // Internal helper to convert numeric prediction to A-Z
    std::string getLabelText(float label) const;
};

#endif // GESTURE_RECOGNIZER_HPP
