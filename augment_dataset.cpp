/**
 * augment_dataset.cpp
 *
 * Expand an existing binary-mask dataset folder to a target image count
 * using data augmentation: rotation, Gaussian noise, and perspective warp.
 *
 * Usage:  ./build/AugmentDataset <LETTER> [target_count]
 * Example: ./build/AugmentDataset A 100
 *          ./build/AugmentDataset V 100
 *
 * The tool reads every .jpg already in dataset/<LETTER>/, cycles through them,
 * and saves augmented copies until the folder contains target_count images.
 * Existing images are never overwritten.
 */

#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace cv;

// ── CONFIG ───────────────────────────────────────────────────────────────────
static constexpr int DEFAULT_TARGET = 100;
static constexpr int THUMB_SIZE     = 50;   // must match IMG_SIZE in train.cpp
// ─────────────────────────────────────────────────────────────────────────────


/** Random rotation ±max_angle degrees around the image centre */
static Mat applyRotation(const Mat& src, std::mt19937& rng, float maxAngle = 15.f) {
    std::uniform_real_distribution<float> dist(-maxAngle, maxAngle);
    float angle = dist(rng);
    Point2f centre(src.cols / 2.f, src.rows / 2.f);
    Mat M = getRotationMatrix2D(centre, angle, 1.0f);
    Mat dst;
    warpAffine(src, dst, M, src.size(),
               INTER_NEAREST, BORDER_CONSTANT, Scalar(0));
    return dst;
}


/** Add salt-and-pepper + Gaussian noise to a binary mask */
static Mat applyNoise(const Mat& src, std::mt19937& rng, float noiseLevel = 0.04f) {
    Mat dst = src.clone();
    std::uniform_real_distribution<float> prob(0.f, 1.f);
    std::normal_distribution<float> gauss(0.f, 20.f);

    for (int y = 0; y < dst.rows; y++) {
        for (int x = 0; x < dst.cols; x++) {
            float p = prob(rng);
            if (p < noiseLevel / 2.f)
                dst.at<uchar>(y, x) = 0;           // pepper
            else if (p < noiseLevel)
                dst.at<uchar>(y, x) = 255;          // salt
            else {
                // Gaussian perturbation on the mask value
                float val = dst.at<uchar>(y, x) + gauss(rng);
                dst.at<uchar>(y, x) = static_cast<uchar>(
                    std::clamp(val, 0.f, 255.f));
            }
        }
    }
    // Re-binarise after noise
    threshold(dst, dst, 127, 255, THRESH_BINARY);
    return dst;
}


/** Slight random perspective warp (simulates tilt/angle variation) */
static Mat applyPerspective(const Mat& src, std::mt19937& rng, float maxShift = 4.f) {
    std::uniform_real_distribution<float> jitter(-maxShift, maxShift);
    float w = static_cast<float>(src.cols);
    float h = static_cast<float>(src.rows);

    // Source corners
    Point2f srcPts[4] = {
        {0,   0},
        {w-1, 0},
        {w-1, h-1},
        {0,   h-1}
    };
    // Destination corners with random jitter
    Point2f dstPts[4] = {
        {jitter(rng),       jitter(rng)},
        {w-1+jitter(rng),   jitter(rng)},
        {w-1+jitter(rng),   h-1+jitter(rng)},
        {jitter(rng),       h-1+jitter(rng)}
    };

    Mat M = getPerspectiveTransform(srcPts, dstPts);
    Mat dst;
    warpPerspective(src, dst, M, src.size(),
                    INTER_NEAREST, BORDER_CONSTANT, Scalar(0));
    return dst;
}


/** Apply a random combination of augmentations */
static Mat augment(const Mat& src, std::mt19937& rng) {
    Mat img = src.clone();

    // Always rotate
    img = applyRotation(img, rng);

    // 70 % chance of perspective warp
    if (std::uniform_real_distribution<float>(0.f, 1.f)(rng) < 0.7f)
        img = applyPerspective(img, rng);

    // 60 % chance of noise
    if (std::uniform_real_distribution<float>(0.f, 1.f)(rng) < 0.6f)
        img = applyNoise(img, rng);

    return img;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <LETTER> [target_count]\n";
        std::cerr << "Example: " << argv[0] << " A 100\n";
        return 1;
    }

    std::string letter    = argv[1];
    int         target    = (argc >= 3) ? std::stoi(argv[2]) : DEFAULT_TARGET;
    std::string folder    = "dataset/" + letter;

    if (!fs::exists(folder)) {
        std::cerr << "[ERROR] Folder not found: " << folder << "\n";
        return 1;
    }

    // Load all existing images
    std::vector<Mat> originals;
    for (auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() == ".jpg") {
            Mat img = imread(entry.path().string(), IMREAD_GRAYSCALE);
            if (!img.empty()) originals.push_back(img);
        }
    }

    int existing = static_cast<int>(originals.size());
    std::cout << "[" << letter << "] Found " << existing << " existing images.\n";

    if (existing == 0) {
        std::cerr << "[ERROR] No .jpg images found in " << folder << "\n";
        return 1;
    }
    if (existing >= target) {
        std::cout << "[" << letter << "] Already at/above target ("
                  << target << "). Nothing to do.\n";
        return 0;
    }

    int needed = target - existing;
    std::cout << "[" << letter << "] Generating " << needed
              << " augmented images (target: " << target << ")...\n";

    std::mt19937 rng(std::random_device{}());
    int count = 0;

    while (count < needed) {
        // Cycle through originals
        const Mat& base = originals[count % existing];

        Mat aug = augment(base, rng);

        // Save at the same resolution as the source image.
        // train.cpp handles the resize to 50x50 during feature extraction,
        // so augmented images must match the original captured size.
        std::string outPath = folder + "/" + std::to_string(existing + count) + ".jpg";
        imwrite(outPath, aug);
        count++;
    }

    std::cout << "[" << letter << "] Done. Total images: "
              << (existing + count) << "\n";
    return 0;
}
