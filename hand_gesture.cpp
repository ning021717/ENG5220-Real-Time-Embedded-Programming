#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <cmath> // Ensure sqrt/pow functions are defined
#include <thread>
#include <mutex>

using namespace std;
using namespace cv;

// Mutex lock for thread synchronization (prevent data race)
mutex mtx;

/**
 * @brief Callback function for frame processing
 * Handles skin color detection, contour extraction, and gesture recognition for each frame
 * @param frame The video frame to be processed
 * @param contours Container to store detected contours
 * @param hand_roi Region of Interest (ROI) for hand detection
 * @param lower_skin Lower HSV threshold for skin color detection
 * @param upper_skin Upper HSV threshold for skin color detection
 */
void processFrame(Mat frame, vector<vector<Point>> contours, Rect hand_roi, Scalar lower_skin, Scalar upper_skin) {
    Mat hsv_frame, mask;
    vector<Vec4i> hierarchy;

    // Convert frame to HSV color space and apply skin color mask
    cvtColor(frame, hsv_frame, COLOR_BGR2HSV);
    inRange(hsv_frame, lower_skin, upper_skin, mask);

    // Noise reduction (close operation first, then open operation)
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(7, 7));
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);
    morphologyEx(mask, mask, MORPH_OPEN, kernel);

    // Find contours in the mask
    findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (!contours.empty()) {
        // Process the largest contour (assumed to be the hand)
        int max_contour_idx = 0;
        double max_area = 0;
        for (int i = 0; i < contours.size(); i++) {
            double area = contourArea(contours[i]);
            // Filter out small contours (area < 2000) to remove noise
            if (area > max_area && area > 2000) {
                max_area = area;
                max_contour_idx = i;
            }
        }

        // Draw the largest contour on the frame
        drawContours(frame, contours, max_contour_idx, Scalar(0, 255, 0), 2);

        // Calculate the center point of the contour using image moments
        Moments m = moments(contours[max_contour_idx]);
        if (m.m00 != 0) { // Avoid division by zero
            Point center(static_cast<int>(m.m10 / m.m00), static_cast<int>(m.m01 / m.m00));
            circle(frame, center, 5, Scalar(255, 255, 0), -1); // Draw center point (solid circle)
        }

        // Gesture detection based on convexity defects
        int finger_count = 0;
        vector<Vec4i> defects;
        convexityDefects(contours[max_contour_idx], contours[max_contour_idx], defects);

        // Detect fingers by analyzing convexity defects
        for (Vec4i d : defects) {
            int start_idx = d[0]; // Start index of the defect
            int end_idx = d[1];   // End index of the defect
            int far_idx = d[2];   // Furthest point index of the defect
            double depth = d[3] / 256.0; // Defect depth (original value needs to be divided by 256)

            // Count as a finger if defect depth exceeds threshold (30)
            if (depth > 30) {
                finger_count++;
            }
        }

        // Display the detected finger count on the frame
        putText(frame, "Finger Count: " + to_string(finger_count), Point(20, 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 3);
    }

    // Display the processed frame
    imshow("Hand Gesture Recognition", frame);
}

/**
 * @brief Thread function for video frame capture
 * Continuously captures frames from camera and dispatches processing tasks
 * @param cap Reference to VideoCapture object (camera handle)
 * @param hand_roi ROI for hand detection
 * @param lower_skin Lower HSV threshold for skin color
 * @param upper_skin Upper HSV threshold for skin color
 */
void captureFrames(VideoCapture &cap, Rect hand_roi, Scalar lower_skin, Scalar upper_skin) {
    Mat frame;
    vector<vector<Point>> contours;

    while (true) {
        bool ret = cap.read(frame);
        if (!ret || frame.empty()) {
            cout << "⚠️ Failed to capture frame, retrying..." << endl;
            continue;
        }

        // Flip frame horizontally (mirror effect, more intuitive for users)
        flip(frame, frame, 1);

        // Create a separate thread for each frame processing
        thread process_thread(processFrame, frame, contours, hand_roi, lower_skin, upper_skin);
        process_thread.detach(); // Asynchronous processing

        // Handle keyboard input
        int key = waitKey(10) & 0xFF;
        if (key == 'q' || key == 27) { // Press 'q' or ESC to exit
            cout << "Exiting program..." << endl;
            break;
        } else if (key == 's') { // Press 's' to save current frame
            imwrite("hand_gesture.jpg", frame);
            cout << "Saved current frame as hand_gesture.jpg" << endl;
        }
    }
}

int main() {
    // Set console output to UTF-8 encoding to avoid Chinese garbled characters
    SetConsoleOutputCP(CP_UTF8);

    // Open camera
    cout << "🔍 Detecting camera..." << endl;
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cout << "⚠️ Camera not found, trying to connect Raspberry Pi Camera Module 2..." << endl;
        cap.open("/dev/video0"); // Device path for Raspberry Pi Camera Module 2
        if (!cap.isOpened()) {
            cout << "❌ Failed to find camera!" << endl;
            return -1;
        }
    }

    // Set camera parameters (resolution and frame rate)
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FPS, 30);

    cout << "✅ Camera opened successfully!" << endl;

    // Define ROI for hand detection (x, y, width, height)
    Rect hand_roi = Rect(100, 50, 440, 380);

    // Define HSV range for skin color detection (adjust for different skin tones)
    Scalar lower_skin = Scalar(5, 30, 80);  
    Scalar upper_skin = Scalar(30, 255, 255);

    // Start video capture thread
    thread capture_thread(captureFrames, ref(cap), hand_roi, lower_skin, upper_skin);
    capture_thread.join(); // Wait for capture thread to finish

    // Release resources
    cap.release();
    destroyAllWindows();
    return 0;
}
