#ifndef CAMERA_MANAGER_HPP
#define CAMERA_MANAGER_HPP

#include <opencv2/opencv.hpp>
#include <functional>

class CameraManager {
public:
    // 定义回调函数：处理好的ROI图像传给下一级
    using FrameCallback = std::function<void(const cv::Mat&)>;

    CameraManager(int index = 0);
    ~CameraManager();

    bool init();
    void startCapture(FrameCallback callback); // 启动循环并执行回调
    void stop();

private:
    cv::VideoCapture cap;
    int cameraIndex;
    bool isRunning;
    cv::Rect roiRect;
};

#endif//
// Created by 金扫帚奖最佳演员 on 2026/2/6.
//

#ifndef OPENCV_TEST_CAMERAMANAGER_H
#define OPENCV_TEST_CAMERAMANAGER_H

#endif //OPENCV_TEST_CAMERAMANAGER_H