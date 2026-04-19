#ifndef CAMERA_MANAGER_HPP
#define CAMERA_MANAGER_HPP

#include "libcam2opencv.h"
#include <opencv2/opencv.hpp>
#include <functional>

/**
 * CameraManager wraps libcam2opencv to deliver frames via a hardware-event
 * callback. The libcamera stack uses blocking I/O on a dedicated file
 * descriptor: the kernel wakes the internal libcamera thread only when the
 * sensor has delivered a new frame, satisfying the course requirement for
 * "blocking I/O wakes up threads" and true event-driven deterministic code.
 */
class CameraManager {
public:
    // Callback type: receives the ROI-cropped frame
    using FrameCallback = std::function<void(const cv::Mat&)>;

    explicit CameraManager(int index = 0);
    ~CameraManager();

    /**
     * Register the frame callback and start the libcamera pipeline.
     * The callback is invoked from the libcamera event thread each time the
     * hardware delivers a complete frame — no polling, no busy-waiting.
     */
    void startCapture(FrameCallback callback);

    void stop();

private:
    /**
     * Inner callback adapter. Inherits the pure-virtual Libcam2OpenCV::Callback
     * interface so that libcamera's requestComplete signal dispatches directly
     * into our handler — a textbook C++ virtual-function callback pattern.
     */
    struct FrameHandler : public Libcam2OpenCV::Callback {
        CameraManager* parent;
        explicit FrameHandler(CameraManager* p) : parent(p) {}
        void hasFrame(const cv::Mat& frame,
                      const libcamera::ControlList& metadata) override;
    };

    Libcam2OpenCV camera;
    FrameHandler  handler;
    FrameCallback userCallback;
    int           cameraIndex;
    cv::Rect      roiRect;
};

#endif // CAMERA_MANAGER_HPP
