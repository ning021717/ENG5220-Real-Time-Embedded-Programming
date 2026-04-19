#include "CameraManager.hpp"
#include <iostream>

CameraManager::CameraManager(int index)
    : cameraIndex(index), roiRect(100, 50, 440, 380)
{}

CameraManager::~CameraManager() {
    stop();
}

void CameraManager::startCapture(FrameCallback callback) {
    userCallback = std::move(callback);

    // Register lambda as the frame callback (OnFrame = std::function).
    // The kernel wakes the libcamera thread via blocking I/O on the V4L2
    // media-controller fd whenever the sensor delivers a complete frame.
    camera.registerCallback([this](const cv::Mat& frame,
                                   const libcamera::ControlList&) {
        if (!userCallback) return;

        const cv::Rect& r = roiRect;
        cv::Mat roi;
        if (r.x + r.width  <= frame.cols &&
            r.y + r.height <= frame.rows)
            roi = frame(r).clone();
        else
            roi = frame.clone();

        // Invoking userCallback wakes the consumer thread blocked on
        // condition_variable::wait() in main — hardware-event-driven chain.
        userCallback(roi);
    });

    Libcam2OpenCVSettings settings;
    settings.cameraIndex = static_cast<unsigned int>(cameraIndex);
    settings.width       = 640;
    settings.height      = 480;
    settings.framerate   = 30;

    cm.start();
    camera.start(cm, settings);
}

void CameraManager::stop() {
    camera.stop();
    cm.stop();
}
