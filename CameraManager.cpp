#include "CameraManager.hpp"
#include <iostream>
#include <chrono>

// Constructor
CameraManager::CameraManager(int index) : cameraIndex(index), isRunning(false) {
    // Define the Region of Interest (ROI)
    roiRect = cv::Rect(100, 50, 440, 380);
}

// Destructor
CameraManager::~CameraManager() {
    stop();
}

// Initialization
bool CameraManager::init() {
    cap.open(cameraIndex, cv::CAP_V4L2);
    
    if (!cap.isOpened()) {
        std::cerr << "[ERROR] Failed to open camera. Please check the connection." << std::endl;
        return false;
    }

    // Lower resolution for real-time processing speed
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    return true;
}

// Start Capture Thread (Producer) with Self-Healing Logic
void CameraManager::startCapture(FrameCallback callback) {
    if (isRunning) return;
    isRunning = true;

    captureThread = std::thread([this, callback]() {
        cv::Mat frame, roi;
        int consecutiveErrors = 0;
        const int MAX_ERRORS = 20; // Tolerance threshold before software reset

        while (isRunning) {
            // Read frame from hardware
            bool success = cap.read(frame);

            // [ROBUSTNESS UPGRADE] Error Handling & Auto-Reconnect
            if (!success || frame.empty()) {
                consecutiveErrors++;
                std::cerr << "[WARNING] Frame drop detected (" << consecutiveErrors << "/" << MAX_ERRORS << ")" << std::endl;
                
                if (consecutiveErrors >= MAX_ERRORS) {
                    std::cerr << "[RECOVERY] Attempting software camera reset..." << std::endl;
                    cap.release();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    
                    // Try to re-open the camera
                    cap.open(cameraIndex, cv::CAP_V4L2);
                    if (cap.isOpened()) {
                        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
                        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
                        consecutiveErrors = 0; // Reset counter on success
                        std::cerr << "[RECOVERY] Camera reset successful!" << std::endl;
                    } else {
                        std::cerr << "[FATAL] Camera unrecoverable. Please check hardware." << std::endl;
                        break; // Truly dead, exit thread
                    }
                } else {
                    // Minor hiccup, wait 50ms and try again
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                continue; // Skip the rest of the loop and try grabbing again
            }

            // Reset error counter if a valid frame is captured
            consecutiveErrors = 0;
            
            //cv::flip(frame, frame, 1); // Mirror effect

            // ROI boundary protection
            if (roiRect.x + roiRect.width <= frame.cols && roiRect.y + roiRect.height <= frame.rows) {
                roi = frame(roiRect).clone();
            } else {
                roi = frame.clone(); 
            }

            // Draw bounding box for visual feedback
            cv::rectangle(frame, roiRect, cv::Scalar(255, 0, 0), 2);

            // Trigger the event callback to wake up the main thread
            if (callback) {
                callback(roi); 
            }
        }
    });
}

// Safely terminate the thread and release hardware
void CameraManager::stop() {
    isRunning = false;
    if (captureThread.joinable()) {
        captureThread.join();
    }
    if (cap.isOpened()) {
        cap.release();
    }
}