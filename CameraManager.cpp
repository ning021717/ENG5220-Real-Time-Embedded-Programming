#include "CameraManager.hpp"
#include <iostream>

CameraManager::CameraManager(int index) : cameraIndex(index), isRunning(false) {
    roiRect = cv::Rect(100, 50, 440, 380); // 定义手部检测区域
}

CameraManager::~CameraManager() { stop(); }

bool CameraManager::init() {
    cap.open(cameraIndex);
    if (!cap.isOpened()) return false;
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    return true;
}

void CameraManager::startCapture(FrameCallback callback) {
    isRunning = true;
    cv::Mat frame, roi;
    while (isRunning) {
        if (!cap.read(frame)) break;
        cv::flip(frame, frame, 1); // 镜像

        // 预处理：裁剪ROI，减轻后续模块计算压力
        roi = frame(roiRect).clone();

        // 执行回调：将图像送往手势识别模块
        if (callback) callback(roi);

        // 显示原始画面（带框）
        cv::rectangle(frame, roiRect, cv::Scalar(255, 0, 0), 2);
        cv::imshow("Camera Stream", frame);

        if (cv::waitKey(10) == 'q') break;
    }
}

void CameraManager::stop() {
    isRunning = false;
    cap.release();
}