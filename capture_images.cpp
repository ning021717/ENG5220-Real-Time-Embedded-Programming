#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <thread>  
#include <mutex>   
#include <atomic>  
#include <cctype>

using namespace cv;
using namespace std;

// ==========================================
// Shared Resources & Thread Control
// ==========================================
Mat shared_frame;               
mutex frame_mutex;              
atomic<bool> keep_running(true); 

// Global variables for HSV Trackbars
int H_MIN = 0, H_MAX = 20;
int S_MIN = 30, S_MAX = 255;
int V_MIN = 30, V_MAX = 255;

void on_trackbar(int, void*) {} // Dummy callback

// ==========================================
// Thread A: Hardware Camera Producer
// ==========================================
void cameraThread() {
    VideoCapture cap(0, cv::CAP_V4L2); 
    if (!cap.isOpened()) {
        cerr << "[ERROR] Failed to open camera." << endl;
        keep_running = false;
        return;
    }

    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);

    Mat temp_frame;
    while (keep_running) {
        if (!cap.read(temp_frame)) {
            this_thread::sleep_for(chrono::milliseconds(10));
            continue; 
        }
        {
            lock_guard<mutex> lock(frame_mutex);
            shared_frame = temp_frame.clone();
        }
    }
    cap.release();
}

// ==========================================
// Thread B: GUI, Mask Processing & Storage
// ==========================================
int main() {
    string label;
    cout << "======================================" << endl;
    cout << "Enter the letter you want to record (A-Z): ";
    cin >> label;
    
    for (auto & c: label) c = toupper(c);
    string folder = "dataset/" + label;
    system(("mkdir -p " + folder).c_str());

    cout << "[INFO] Starting hardware camera thread..." << endl;
    thread cam_thread(cameraThread);

    // Create GUI for HSV adjustment
    namedWindow("Raw Camera", WINDOW_AUTOSIZE);
    namedWindow("Binary Mask (Data to Save)", WINDOW_AUTOSIZE);
    createTrackbar("H Min", "Binary Mask (Data to Save)", &H_MIN, 179, on_trackbar);
    createTrackbar("H Max", "Binary Mask (Data to Save)", &H_MAX, 179, on_trackbar);
    createTrackbar("S Min", "Binary Mask (Data to Save)", &S_MIN, 255, on_trackbar);
    createTrackbar("S Max", "Binary Mask (Data to Save)", &S_MAX, 255, on_trackbar);

    int count = 0;
    cout << "[Target]: " << label << endl;
    cout << "[Controls]: Adjust trackbars until background is black and hand is white." << endl;
    cout << "            Press 's' to save the MASK, 'q' to exit" << endl;

    Mat frame, hsv, mask; 
    cv::Rect guideRect(100, 50, 440, 380);
    
    while (keep_running) {
        {
            lock_guard<mutex> lock(frame_mutex);
            if (shared_frame.empty()) continue; 
            frame = shared_frame.clone();
        }
        //cv::flip(frame, frame, 1);

        // 1. Crop to ROI
        Mat roi;
        if (guideRect.x + guideRect.width <= frame.cols && guideRect.y + guideRect.height <= frame.rows) {
            roi = frame(guideRect).clone();
        } else {
            roi = frame.clone(); 
        }

        // 2. Generate Binary Mask
        cvtColor(roi, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), mask);

        // Morphological cleaning
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        erode(mask, mask, kernel);
        dilate(mask, mask, kernel);

        // 3. Display Guiding Box on Raw Frame
        cv::rectangle(frame, guideRect, cv::Scalar(0, 255, 255), 2);
        imshow("Raw Camera", frame);
        imshow("Binary Mask (Data to Save)", mask);

        char key = (char)waitKey(1);
        if (key == 's') {
            // [CRITICAL FIX] We save the Binary MASK, not the raw image!
            string filename = folder + "/" + to_string(count) + ".jpg";
            imwrite(filename, mask);
            cout << "[SUCCESS] Saved pure mask: " << filename << endl;
            count++; 
        } else if (key == 'q' || key == 27) { 
            keep_running = false; 
            break;
        }
    }

    if (cam_thread.joinable()) cam_thread.join();
    destroyAllWindows();
    return 0;
}