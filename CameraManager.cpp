#include "CameraManager.hpp"
#include <iostream>

CameraManager::CameraManager(int index)
    : handler(this), cameraIndex(index), roiRect(100, 50, 440, 380)
{}

CameraManager::~CameraManager() {
    stop();
}

void CameraManager::startCapture(FrameCallback callback) {
    userCallback = std::move(callback);
    camera.registerCallback(&handler);

    Libcam2OpenCVSettings settings;
    settings.cameraIndex = static_cast<unsigned int>(cameraIndex);
    settings.width       = 640;
    settings.height      = 480;
    settings.framerate   = 30;

    // start() hands control to libcamera's internal event loop.
    // The kernel wakes the libcamera thread via blocking I/O (a poll/select on
    // the V4L2 media-controller file descriptor) whenever the image sensor has
    // captured a complete frame. This is the "blocking I/O wakes up threads"
    // pattern required by the course.
    camera.start(settings);
}

void CameraManager::stop() {
    camera.stop();
}

// Called from the libcamera event thread exactly once per hardware frame.
// No polling — execution reaches here only because the kernel unblocked the
// thread after the sensor's DMA transfer completed.
void CameraManager::FrameHandler::hasFrame(const cv::Mat& frame,
                                           const libcamera::ControlList&) {
    if (!parent->userCallback) return;

    const cv::Rect& r = parent->roiRect;
    cv::Mat roi;
    if (r.x + r.width  <= frame.cols &&
        r.y + r.height <= frame.rows) {
        roi = frame(r).clone();
    } else {
        roi = frame.clone();
    }

    // Invoking the user callback wakes up the consumer thread that is blocked
    // on condition_variable::wait() in main — completing the producer-consumer
    // event chain entirely driven by hardware interrupts.
    parent->userCallback(roi);
}
