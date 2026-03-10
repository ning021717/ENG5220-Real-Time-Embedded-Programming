#include <vector>
#include <deque>
#include <cmath>
#include <functional>
#include <opencv2/opencv.hpp>

// Forward declaration for callback type and SignDatabase
using RecognitionCallback = std::function<void(const std::string&)>;
class SignDatabase;

class GestureRecognizer {
public:
    GestureRecognizer() {}

    // Feature extraction implementation
    std::vector<float> extractFeatures(const cv::Mat& frame);

    // DTW (Dynamic Time Warping) algorithm implementation
    float calculateDTW(const std::deque<std::vector<float>>& seq, const std::vector<float>& templ);

    // Main gesture recognition processing pipeline
    void process(const cv::Mat& frame, const SignDatabase& db, RecognitionCallback callback);

private:
    // Circular buffer to store feature sequences (max 30 frames)
    std::deque<std::vector<float>> sequenceBuffer;
};

// Constructor (empty implementation)
GestureRecognizer::GestureRecognizer() {}

/**
 * @brief Extract features from a single frame for gesture recognition
 * @param frame Input frame (cv::Mat) from camera/video stream
 * @return Vector of float features (simulated: 21 keypoints × 3 coordinates = 63 dimensions)
 * @note In practical applications: replace simulation with real feature extraction (e.g., centroid via color thresholding)
 */
std::vector<float> GestureRecognizer::extractFeatures(const cv::Mat& frame) {
    // Simulate feature vector: 21 keypoints × 3 coordinates (x, y, z)
    // For real-world use: extract centroid using color thresholding as simplified features
    return std::vector<float>(63, 0.5f);
}

/**
 * @brief Calculate Dynamic Time Warping (DTW) distance between two feature sequences
 * @param seq Input feature sequence (deque of frame-level feature vectors)
 * @param templ Template feature sequence (flattened into a single vector)
 * @return Total DTW distance (sum of absolute differences between matched features)
 */
float GestureRecognizer::calculateDTW(const std::deque<std::vector<float>>& seq, const std::vector<float>& templ) {
    float totalDist = 0.0f;
    for (size_t i = 0; i < seq.size(); ++i) {
        for (int k = 0; k < 63; ++k) {
            // Boundary check to avoid out-of-bounds access to template vector
            if (i * 63 + k < templ.size()) {
                // Accumulate absolute difference of corresponding feature dimensions
                totalDist += std::abs(seq[i][k] - templ[i * 63 + k]);
            }
        }
    }
    return totalDist;
}

/**
 * @brief Main processing pipeline for real-time gesture recognition
 * @param frame Input frame to process
 * @param db Reference to sign database (contains gesture templates for matching)
 * @param callback Callback function to return recognition result (e.g., "ActionA")
 * @note The buffer holds up to 30 frames of features (circular buffer behavior)
 */
void GestureRecognizer::process(const cv::Mat& frame, const SignDatabase& db, RecognitionCallback callback) {
    // Step 1: Extract features from current frame
    std::vector<float> feats = extractFeatures(frame);
    
    // Step 2: Add new features to sequence buffer (maintain max 30 frames)
    sequenceBuffer.push_back(feats);
    if (sequenceBuffer.size() > 30) {
        sequenceBuffer.pop_front(); // Remove oldest frame when buffer is full
    }

    // Step 3: Perform gesture matching when buffer is full (30 frames collected)
    if (sequenceBuffer.size() == 30) {
        // TODO: Add gesture matching logic here (e.g., use calculateDTW to match with templates in db)
        // Example: callback("ActionA"); // Return recognition result via callback
    }
}
