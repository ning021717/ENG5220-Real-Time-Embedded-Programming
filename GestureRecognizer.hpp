#ifndef GESTURE_RECOGNIZER_HPP
#define GESTURE_RECOGNIZER_HPP

#include <opencv2/opencv.hpp>
#include <deque>
#include <vector>
#include <numeric>

// 手语模板结构体
struct SignTemplate {
    std::string actionName;
    std::vector<float> features; // 存储 21*3 个关键点特征
};

class GestureRecognizer {
public:
    using RecognitionCallback = std::function<void(std::string)>;

    GestureRecognizer() {
        loadSignLibrary();
    }

    // 接收新的一帧，提取特征并存入 30 帧缓冲区
    void process(const cv::Mat& roi, RecognitionCallback cb) {
        std::vector<float> currentFeatures = extractKeypoints(roi);

        // 模拟 sequence.append()
        sequenceBuffer.push_back(currentFeatures);
        if (sequenceBuffer.size() > 30) sequenceBuffer.pop_front();

        // 缓冲区满 30 帧后执行匹配
        if (sequenceBuffer.size() == 30) {
            std::string result = matchLibrary();
            if (result != "Unknown" && cb) cb(result);
        }
    }

private:
    std::deque<std::vector<float>> sequenceBuffer;
    std::vector<SignTemplate> signLibrary;

    // 读取手语库：从磁盘加载特征
    void loadSignLibrary() {
        // 实际开发中，这里可以使用 cv::FileStorage 读取 .xml 或 .yml
        // 这里手动模拟加载 A, B 两个手语的特征模板
        signLibrary.push_back({"Action_A", {0.1f, 0.2f /* ...63个特征... */}});
        signLibrary.push_back({"Action_B", {0.5f, 0.8f /* ...63个特征... */}});
    }

    std::vector<float> extractKeypoints(cv::Mat roi) {
        // 这里集成你的特征提取算法（如轮廓、中心距等）
        // 对应 Python 的 extract_keypoints(results)
        return std::vector<float>(63, 0.0f);
    }

    std::string matchLibrary() {
        // 计算当前序列与库中哪个模板最接近
        // 简化版：计算当前帧与模板的相似度
        for (const auto& sign : signLibrary) {
            float dist = calculateDistance(sequenceBuffer.back(), sign.features);
            if (dist < 0.5f) return sign.actionName; // 阈值判定
        }
        return "Unknown";
    }

    float calculateDistance(const std::vector<float>& v1, const std::vector<float>& v2) {
        float sum = 0;
        for (size_t i = 0; i < v1.size(); ++i) sum += std::pow(v1[i] - v2[i], 2);
        return std::sqrt(sum);
    }
};
#endif
// Created by 金扫帚奖最佳演员 on 2026/2/6.
//

#ifndef OPENCV_TEST_GESTURERECOGNIZER_H
#define OPENCV_TEST_GESTURERECOGNIZER_H

#endif //OPENCV_TEST_GESTURERECOGNIZER_H