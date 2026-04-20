#include "GestureRecognizer.hpp"
#include <algorithm>
#include <iostream>

// ── Constructor ───────────────────────────────────────────────────────────────
// Auto-selects backend from the file extension:
//   .onnx → CNN via cv::dnn  (position/scale invariant, ~5–10 ms on Pi 4)
//   .xml  → KNN via cv::ml   (legacy fallback)
GestureRecognizer::GestureRecognizer(const std::string& modelPath) {
    bool isOnnx = modelPath.size() >= 5 &&
                  modelPath.substr(modelPath.size() - 5) == ".onnx";

    if (isOnnx) {
        cnnNet = cv::dnn::readNetFromONNX(modelPath);
        if (cnnNet.empty()) {
            std::cerr << "[ERROR] Failed to load CNN model from: "
                      << modelPath << std::endl;
        } else {
            cnnNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            cnnNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            useCNN = true;
            std::cout << "[INFO] CNN backend loaded: " << modelPath << std::endl;
        }
    } else {
        knn = cv::ml::KNearest::load(modelPath);
        if (knn.empty()) {
            std::cerr << "[ERROR] Failed to load KNN model from: "
                      << modelPath << std::endl;
        } else {
            std::cout << "[INFO] KNN backend loaded: " << modelPath << std::endl;
        }
    }
}

bool GestureRecognizer::isModelLoaded() const {
    return useCNN ? !cnnNet.empty() : !knn.empty();
}

std::string GestureRecognizer::backendName() const {
    return useCNN ? "CNN" : "KNN";
}

std::string GestureRecognizer::getLabelText(float label) const {
    int i = static_cast<int>(label);
    if (i >= 0 && i < 26)
        return std::string(1, static_cast<char>('A' + i));
    return "Unknown";
}

// ── Shared preprocessing ──────────────────────────────────────────────────────
// Steps 1–4 are identical for both backends.
std::string GestureRecognizer::predict(const cv::Mat& roi, cv::Mat& outMask) {
    if (roi.empty() || !isModelLoaded())
        return "Error";

    cv::Mat ycrcb;

    // 1. YCrCb Skin Segmentation
    // YCrCb separates luminance (Y) from chrominance (Cr, Cb), making skin
    // detection robust to lighting changes — bright or dim light only shifts Y,
    // while skin Cr/Cb values stay stable across illumination conditions.
    cv::cvtColor(roi, ycrcb, cv::COLOR_BGR2YCrCb);
    cv::inRange(ycrcb,
                cv::Scalar(0,   CR_MIN, CB_MIN),
                cv::Scalar(255, CR_MAX, CB_MAX),
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
    if (cv::countNonZero(outMask) < 1000)
        return "No Hand";

    // 4. Bounding-Box Normalisation
    // Resizing the full ROI directly makes the feature vector position- and
    // scale-dependent: the same hand shape placed in the top-left corner
    // produces a completely different 50×50 pixel vector than when placed in
    // the centre, causing classifiers to identify hand position rather than
    // hand shape. Cropping to the tight bounding box of the largest skin blob
    // first makes features invariant to both position and size within the ROI.
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(outMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) return "No Hand";

    auto maxIt = std::max_element(contours.begin(), contours.end(),
        [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
            return cv::contourArea(a) < cv::contourArea(b);
        });
    cv::Rect bbox = cv::boundingRect(*maxIt);

    // Small padding so fingertips at the bbox edge are not clipped.
    const int pad = 4;
    bbox.x      = std::max(0, bbox.x - pad);
    bbox.y      = std::max(0, bbox.y - pad);
    bbox.width  = std::min(outMask.cols - bbox.x, bbox.width  + 2 * pad);
    bbox.height = std::min(outMask.rows - bbox.y, bbox.height + 2 * pad);

    // 5. Crop and resize to the canonical 50×50 input
    cv::Mat normalised;
    cv::resize(outMask(bbox), normalised, cv::Size(IMG_SIZE, IMG_SIZE));

    // 6. Backend-specific inference
    return useCNN ? predictCNN(normalised) : predictKNN(normalised);
}

// ── KNN Inference ─────────────────────────────────────────────────────────────
std::string GestureRecognizer::predictKNN(const cv::Mat& normalised50x50) const {
    // Flatten to a single 1×2500 float32 row vector for KNearest::findNearest.
    cv::Mat feat = normalised50x50.clone().reshape(1, 1);
    feat.convertTo(feat, CV_32F);

    // Vote-based confidence: fraction of k=9 neighbours that agree.
    // Distance-based guard: reject inputs that are too far from any training
    // sample (squared L2 threshold ~1.5×10^7 is empirical for 50×50 masks).
    cv::Mat neighborResponses, dists;
    float result = knn->findNearest(feat, 9, cv::noArray(),
                                    neighborResponses, dists);

    int votes = 0;
    for (int i = 0; i < neighborResponses.cols; i++)
        if (neighborResponses.at<float>(0, i) == result) votes++;
    float confidence = static_cast<float>(votes) / neighborResponses.cols;
    float minDist    = dists.at<float>(0, 0);

    if (confidence < 0.8f || minDist > 1.5e7f)
        return "Uncertain";

    return getLabelText(result);
}

// ── CNN Inference ─────────────────────────────────────────────────────────────
std::string GestureRecognizer::predictCNN(const cv::Mat& normalised50x50) {
    // Build a 4-D float32 blob: (1, 1, 50, 50), pixel values scaled to [0,1].
    // This matches the training normalisation in train_cnn.py.
    cv::Mat blob = cv::dnn::blobFromImage(
        normalised50x50,
        1.0 / 255.0,                       // scale to [0, 1]
        cv::Size(IMG_SIZE, IMG_SIZE),
        cv::Scalar(0),
        /*swapRB=*/false,
        /*crop=*/false,
        CV_32F);

    cnnNet.setInput(blob);
    cv::Mat logits = cnnNet.forward();     // shape: (1, 26)
    logits = logits.reshape(1, 1);         // flatten to 1×26 row

    // Stable softmax: subtract max before exp to prevent numerical overflow.
    double maxLogit;
    cv::minMaxLoc(logits, nullptr, &maxLogit);
    cv::Mat shifted;
    cv::exp(logits - static_cast<float>(maxLogit), shifted);
    shifted /= static_cast<float>(cv::sum(shifted)[0]);  // now softmax probs

    // Pick the class with highest probability.
    cv::Point maxLoc;
    double    maxProb;
    cv::minMaxLoc(shifted, nullptr, &maxProb, nullptr, &maxLoc);

    // Reject predictions where the model is not confident enough.
    if (maxProb < 0.70)
        return "Uncertain";

    return getLabelText(static_cast<float>(maxLoc.x));
}
