#ifndef CAMERA_MANAGER_HPP
#define CAMERA_MANAGER_HPP

#include <opencv2/opencv.hpp>
#include <functional>
#include <thread>
#include <atomic>

class CameraManager {
public:
    // 定义回调类型
    using FrameCallback = std::function<void(const cv::Mat&)>;

    CameraManager(int index = 0);
    ~CameraManager();

    bool init();
    void startCapture(FrameCallback callback);
    void stop();

private:
    cv::VideoCapture cap;
    int cameraIndex;
    std::thread captureThread;
    std::atomic<bool> isRunning;
    cv::Rect roiRect; // 修复：在头文件中声明 roiRect
};

#endif