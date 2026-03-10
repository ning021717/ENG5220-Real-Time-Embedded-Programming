#ifndef CAMERA_MANAGER_HPP
#define CAMERA_MANAGER_HPP

#include <opencv2/opencv.hpp>
#include <functional>
#include <thread>
#include <atomic>

class CameraManager {
public:
    // Define callback type for frame processing
    // The callback receives a const reference to the captured frame (cv::Mat)
    using FrameCallback = std::function<void(const cv::Mat&)>;

    /**
     * @brief Constructor for CameraManager
     * @param index Camera device index (default: 0 for the first camera)
     */
    CameraManager(int index = 0);

    /**
     * @brief Destructor for CameraManager
     * Ensures proper cleanup (stops capture thread, releases camera resource)
     */
    ~CameraManager();

    /**
     * @brief Initialize the camera capture
     * @return true if camera is initialized successfully, false otherwise
     */
    bool init();

    /**
     * @brief Start continuous frame capture in a separate thread
     * @param callback The function to be called for each captured frame
     */
    void startCapture(FrameCallback callback);

    /**
     * @brief Stop the frame capture and release resources
     * Terminates the capture thread and releases the VideoCapture object
     */
    void stop();

private:
    cv::VideoCapture cap;          // OpenCV VideoCapture object for camera access
    int cameraIndex;               // Index of the target camera device
    std::thread captureThread;     // Thread for asynchronous frame capture
    std::atomic<bool> isRunning;   // Atomic flag to control capture thread execution
    cv::Rect roiRect;              // Fix: Declare roiRect in the header file (ROI region for frame cropping)
};

#endif // CAMERA_MANAGER_H  // End of header guard
