#include "CameraManager.hpp"
#include <iostream>

// Constructor
CameraManager::CameraManager(int index) : cameraIndex(index), isRunning(false) {
    // initialization ROI 
    roiRect = cv::Rect(100, 50, 440, 380);
}

// Constructor
CameraManager::~CameraManager() {
    stop();
}

// initialization
bool CameraManager::init() {
    cap.open(cameraIndex);
    if (!cap.isOpened()) return false;

    // Adjust parameters to lower the resolution and increase the frame rate
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    return true;
}

// start capture
void CameraManager::startCapture(FrameCallback callback) {
    if (isRunning) return;
    isRunning = true;

    // Start a separate thread to avoid blocking the main program.
    captureThread = std::thread([this, callback]() {
        cv::Mat frame, roi;
        while (isRunning) {
            if (!cap.read(frame)) break;
            cv::flip(frame, frame, 1); // 镜像

            // Preprocessing
            if (roiRect.x + roiRect.width <= frame.cols && roiRect.y + roiRect.height <= frame.rows) {
                roi = frame(roiRect).clone();
            } else {
                roi = frame.clone(); // 保护逻辑
            }

            // The drawing box facilitates debugging
            cv::rectangle(frame, roiRect, cv::Scalar(255, 0, 0), 2);

            // Execute callback: Pass out the processed ROI.
            if (callback) {
                callback(roi); 
            }

            // preventing 100% CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    });
}

// stop capture
void CameraManager::stop() {
    isRunning = false;
    if (captureThread.joinable()) {
        captureThread.join();
    }
    if (cap.isOpened()) {
        cap.release();
    }

}
