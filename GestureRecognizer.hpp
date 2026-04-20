#ifndef GESTURE_RECOGNIZER_HPP
#define GESTURE_RECOGNIZER_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <opencv2/dnn.hpp>
#include <string>

class GestureRecognizer {
public:
    // Constructor: auto-detects backend from file extension.
    //   *.onnx  → CNN via cv::dnn::readNetFromONNX  (preferred)
    //   *.xml   → KNN via cv::ml::KNearest::load    (fallback)
    explicit GestureRecognizer(const std::string& modelPath);
    ~GestureRecognizer() = default;

    // Returns true if the model was loaded successfully.
    bool isModelLoaded() const;

    // Returns the backend in use: "CNN" or "KNN".
    std::string backendName() const;

    // Core inference: segments the ROI, classifies the hand gesture, and
    // writes the binary skin mask to outMask.  Returns "A"–"Z", "No Hand",
    // "Uncertain", or "Error".
    std::string predict(const cv::Mat& roi, cv::Mat& outMask);

    // YCrCb skin-segmentation thresholds.
    // Design rationale for public access: OpenCV's createTrackbar() requires a
    // raw int* pointer so that the GUI slider can modify the value in-place on
    // every drag event — there is no callback-based setter variant.  Making
    // these four ints public eliminates the need for global variables in
    // main.cpp while still giving the trackbar direct write access.  All other
    // internal state (knn, cnnNet, IMG_SIZE, getLabelText) remains private;
    // these ints are the minimal public surface required by the OpenCV trackbar API.
    // Y: luminance (ignored for skin), Cr: red-diff, Cb: blue-diff.
    // Typical skin range under varied lighting: Cr [133,173], Cb [77,127].
    int CR_MIN = 133;
    int CR_MAX = 173;
    int CB_MIN = 77;
    int CB_MAX = 127;

private:
    // ── KNN backend ──────────────────────────────────────────────────────────
    cv::Ptr<cv::ml::KNearest> knn;

    // ── CNN backend (OpenCV DNN, loads ONNX) ─────────────────────────────────
    cv::dnn::Net cnnNet;
    bool         useCNN = false;

    const int IMG_SIZE = 50;  // must match training (train.cpp / train_cnn.py)

    std::string predictKNN(const cv::Mat& normalised50x50) const;
    std::string predictCNN(const cv::Mat& normalised50x50);
    std::string getLabelText(float label) const;
};

#endif // GESTURE_RECOGNIZER_HPP
