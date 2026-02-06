#include "GestureRecognizer.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

GestureRecognizer::GestureRecognizer() {}

// 特征提取实现
std::vector<float> GestureRecognizer::extractFeatures(const cv::Mat& frame) {
    // 模拟 21 个关键点 * 3 坐标
    // 在真实作业中，这里可以用颜色阈值提取重心，作为简化特征
    return std::vector<float>(63, 0.5f);
}

// DTW 算法实现
float GestureRecognizer::calculateDTW(const std::deque<std::vector<float>>& seq, const std::vector<float>& templ) {
    float totalDist = 0.0f;
    for (size_t i = 0; i < seq.size(); ++i) {
        for (int k = 0; k < 63; ++k) {
            if (i * 63 + k < templ.size()) {
                totalDist += std::abs(seq[i][k] - templ[i * 63 + k]);
            }
        }
    }
    return totalDist;
}

// 处理流程
void GestureRecognizer::process(const cv::Mat& frame, const SignDatabase& db, RecognitionCallback callback) {
    std::vector<float> feats = extractFeatures(frame);
    sequenceBuffer.push_back(feats);
    if (sequenceBuffer.size() > 30) sequenceBuffer.pop_front();

    if (sequenceBuffer.size() == 30) {
        // 这里可以添加匹配逻辑
        // callback("ActionA");
    }
}