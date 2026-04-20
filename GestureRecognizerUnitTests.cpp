#include "GestureRecognizer.hpp"

#include <opencv2/opencv.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
    namespace fs = std::filesystem;

    // Probe a list of candidate paths and return the first that exists.
    std::string findFile(const std::vector<std::string>& names) {
        const fs::path cwd = fs::current_path();
        for (const auto& name : names) {
            for (const fs::path base : {cwd, cwd / "..", cwd / "../.."}) {
                fs::path p = base / name;
                if (fs::exists(p)) return p.string();
            }
            // bare relative path
            if (fs::exists(name)) return name;
        }
        return {};
    }

    int fail(const std::string& msg) {
        std::cerr << msg << "\n";
        return 1;
    }

    // Run the two model-agnostic behavioural tests against a loaded recognizer.
    int runBehaviourTests(GestureRecognizer& rec, const std::string& tag) {
        // 1) Empty ROI must deterministically yield "Error"
        {
            cv::Mat empty, dummyMask;
            if (rec.predict(empty, dummyMask) != "Error")
                return fail("[FAIL][" + tag + "] Empty ROI did not return \"Error\"");
        }

        // 2) All-black ROI: no skin pixels → "No Hand"
        {
            cv::Mat black = cv::Mat::zeros(200, 200, CV_8UC3);
            cv::Mat outMask;
            if (rec.predict(black, outMask) != "No Hand")
                return fail("[FAIL][" + tag + "] Black ROI did not return \"No Hand\"");
        }

        std::cout << "[PASS][" << tag << "] Behaviour tests\n";
        return 0;
    }
} // namespace


int main() {
    int failures = 0;

    // ── Test 1: KNN backend ───────────────────────────────────────────────────
    {
        const std::string knnPath = findFile({"knn_model.xml"});
        if (knnPath.empty()) {
            std::cerr << "[SKIP] knn_model.xml not found — skipping KNN tests.\n";
        } else {
            GestureRecognizer rec(knnPath);
            if (!rec.isModelLoaded())
                failures += fail("[FAIL][KNN] Failed to load knn_model.xml");
            else if (rec.backendName() != "KNN")
                failures += fail("[FAIL][KNN] backendName() returned wrong value");
            else
                failures += runBehaviourTests(rec, "KNN");
        }
    }

    // ── Test 2: CNN backend (ONNX) ────────────────────────────────────────────
    // Only runs when gesture_cnn.onnx is present in the project root.
    // In CI the ONNX file is absent so this block is silently skipped;
    // on the Raspberry Pi (or after training) it exercises the full CNN path.
    {
        const std::string cnnPath = findFile({"gesture_cnn.onnx"});
        if (cnnPath.empty()) {
            std::cout << "[SKIP] gesture_cnn.onnx not found — skipping CNN tests "
                         "(run train_cnn.py first).\n";
        } else {
            GestureRecognizer rec(cnnPath);
            if (!rec.isModelLoaded())
                failures += fail("[FAIL][CNN] Failed to load gesture_cnn.onnx");
            else if (rec.backendName() != "CNN")
                failures += fail("[FAIL][CNN] backendName() returned wrong value");
            else
                failures += runBehaviourTests(rec, "CNN");
        }
    }

    if (failures == 0)
        std::cout << "[PASS] GestureRecognizerUnitTests\n";
    return failures > 0 ? 1 : 0;
}
