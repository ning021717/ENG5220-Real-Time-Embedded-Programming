#ifndef CAMERA_MANAGER_HPP
#define CAMERA_MANAGER_HPP

#include <opencv2/opencv.hpp>
#include <functional>
#include <thread>
#include <atomic>

class CameraManager {
public:
    using FrameCallback = std::function<void(const cv::Mat&)>;

    CameraManager(int index = 0) : cameraIndex(index), isRunning(false) {}
    ~CameraManager() { stop(); }

    bool init() {
        cap.open(cameraIndex);
        return cap.isOpened();
    }

    void startCapture(FrameCallback callback) {
        isRunning = true;
        // 开启独立线程读取摄像头，保证主界面不卡顿
        captureThread = std::thread([this, callback]() {
            cv::Mat frame;
            while (isRunning) {
                if (cap.read(frame)) {
                    cv::flip(frame, frame, 1);
                    if (callback) callback(frame);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        });
    }

    void stop() {
        isRunning = false;
        if (captureThread.joinable()) captureThread.join();
        cap.release();
    }

private:
    cv::VideoCapture cap;
    int cameraIndex;
    std::thread captureThread;
    std::atomic<bool> isRunning;
};
#endif