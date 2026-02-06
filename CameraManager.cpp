#include "CameraManager.hpp"
#include <iostream>

// 构造函数
CameraManager::CameraManager(int index) : cameraIndex(index), isRunning(false) {
    // 初始化 ROI 区域
    roiRect = cv::Rect(100, 50, 440, 380);
}

// 析构函数
CameraManager::~CameraManager() {
    stop();
}

// 初始化
bool CameraManager::init() {
    cap.open(cameraIndex);
    if (!cap.isOpened()) return false;

    // 设置参数，降低分辨率以提高帧率（符合实时性要求）
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    return true;
}

// 开启采集（多线程版本）
void CameraManager::startCapture(FrameCallback callback) {
    if (isRunning) return;
    isRunning = true;

    // 启动独立线程，不阻塞主程序
    captureThread = std::thread([this, callback]() {
        cv::Mat frame, roi;
        while (isRunning) {
            if (!cap.read(frame)) break;
            cv::flip(frame, frame, 1); // 镜像

            // 预处理
            if (roiRect.x + roiRect.width <= frame.cols && roiRect.y + roiRect.height <= frame.rows) {
                roi = frame(roiRect).clone();
            } else {
                roi = frame.clone(); // 保护逻辑
            }

            // 绘制框方便调试 (可选，会画在原图上)
            cv::rectangle(frame, roiRect, cv::Scalar(255, 0, 0), 2);

            // 执行回调：将处理好的 ROI 传出去
            if (callback) {
                callback(roi); // 传出 ROI 用于识别
                // 如果你想在主界面显示原图，可以通过回调传出 frame，或者在 main 中显示
            }

            // 简单的帧率控制，避免占用 100% CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    });
}

// 停止采集
void CameraManager::stop() {
    isRunning = false;
    if (captureThread.joinable()) {
        captureThread.join();
    }
    if (cap.isOpened()) {
        cap.release();
    }
}