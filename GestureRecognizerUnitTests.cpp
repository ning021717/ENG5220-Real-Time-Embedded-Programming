#include "GestureRecognizer.hpp"

#include <opencv2/opencv.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
    std::string findModelPath() {
        namespace fs = std::filesystem;

        const fs::path cwd = fs::current_path();
        const std::vector<fs::path> candidates = {
            // Common: test run directory is project root
            cwd / "knn_model.xml",
            // Common: test run directory is build/
            cwd / ".." / "knn_model.xml",
            cwd / ".." / ".." / "knn_model.xml",
            // Fallback: plain relative path
            "knn_model.xml",
        };

        for (const auto& p : candidates) {
            if (fs::exists(p)) {
                return p.string();
            }
        }
        return {};
    }

    int fail(const std::string& msg) {
        std::cerr << msg << std::endl;
        return 1;
    }
} // namespace

int main() {
    const std::string modelPath = findModelPath();
    if (modelPath.empty()) {
        return fail("[FAIL] knn_model.xml not found. Ensure it exists in project root.");
    }

    GestureRecognizer recognizer(modelPath);
    if (!recognizer.isModelLoaded()) {
        return fail("[FAIL] GestureRecognizer failed to load knn_model.xml");
    }

    // 1) Empty ROI must deterministically yield "Error"
    {
        cv::Mat empty;
        cv::Mat dummyMask;
        const std::string out = recognizer.predict(empty, dummyMask);
        if (out != "Error") return fail("[FAIL] Empty ROI did not return \"Error\"");
    }

    // 2) All-black ROI should not match "hand" threshold -> "No Hand"
    {
        cv::Mat black = cv::Mat::zeros(200, 200, CV_8UC3);
        cv::Mat outMask;
        const std::string out = recognizer.predict(black, outMask);
        if (out != "No Hand") return fail("[FAIL] Black ROI did not return \"No Hand\"");
    }

    std::cout << "[PASS] GestureRecognizerUnitTests" << std::endl;
    return 0;
}

