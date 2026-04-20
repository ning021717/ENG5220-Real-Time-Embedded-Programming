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
        const fs::path searchRoots[] = {cwd, cwd / "..", cwd / "../.."};
        for (const auto& name : names) {
            for (const fs::path& base : searchRoots) {
                fs::path p = base / name;
                if (fs::exists(p)) return p.string();
            }
            if (fs::exists(name)) return name;
        }
        return {};
    }

    int fail(const std::string& msg) {
        std::cerr << msg << "\n";
        return 1;
    }

    int runBehaviourTests(GestureRecognizer& rec, const std::string& tag) {
        {
            cv::Mat empty, dummyMask;
            if (rec.predict(empty, dummyMask) != "Error")
                return fail("[FAIL][" + tag + "] Empty ROI did not return \"Error\"");
        }
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
    int  failures = 0;
    bool ranKnn   = false;
    bool ranCnn   = false;

    // ── KNN backend (optional file) ───────────────────────────────────────────
    {
        const std::string knnPath = findFile({"knn_model.xml"});
        if (knnPath.empty()) {
            std::cerr << "[SKIP] knn_model.xml not found — skipping KNN tests.\n";
        } else {
            ranKnn = true;
            GestureRecognizer rec(knnPath);
            if (!rec.isModelLoaded())
                failures += fail("[FAIL][KNN] Failed to load knn_model.xml");
            else if (rec.backendName() != "KNN")
                failures += fail("[FAIL][KNN] backendName() returned wrong value");
            else
                failures += runBehaviourTests(rec, "KNN");
        }
    }

    // ── CNN backend (ONNX) — committed as gesture_cnn.onnx for CI + Pi ───────
    {
        const std::string cnnPath = findFile({"gesture_cnn.onnx"});
        if (cnnPath.empty()) {
            std::cerr << "[SKIP] gesture_cnn.onnx not found — skipping CNN tests.\n";
        } else {
            ranCnn = true;
            GestureRecognizer rec(cnnPath);
            if (!rec.isModelLoaded())
                failures += fail("[FAIL][CNN] Failed to load gesture_cnn.onnx");
            else if (rec.backendName() != "CNN")
                failures += fail("[FAIL][CNN] backendName() returned wrong value");
            else
                failures += runBehaviourTests(rec, "CNN");
        }
    }

    if (!ranKnn && !ranCnn) {
        return fail(
            "[FAIL] No model file found. Add gesture_cnn.onnx and/or knn_model.xml "
            "to the repository root, or run this test from build/ with models in ../");
    }

    if (failures == 0)
        std::cout << "[PASS] GestureRecognizerUnitTests\n";
    return failures > 0 ? 1 : 0;
}
