#include "libcam2opencv.h"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cctype>

using namespace cv;
using namespace std;

// ==========================================
// Shared state between libcamera thread and GUI thread
// ==========================================
Mat              shared_frame;
mutex            frame_mutex;
condition_variable frame_cv;
bool             frame_ready  = false;
atomic<bool>     keep_running(true);

int CR_MIN = 133, CR_MAX = 173;
int CB_MIN = 77,  CB_MAX = 127;

void on_trackbar(int, void*) {}

// ==========================================
// Main / GUI thread — blocks on condition_variable until libcamera delivers
// ==========================================
int main() {
    string label;
    cout << "======================================" << endl;
    cout << "Enter the letter you want to record (A-Z): ";
    cin >> label;
    for (auto& c : label) c = toupper(c);

    string folder = "dataset/" + label;
    std::filesystem::create_directories(folder);

    Libcam2OpenCV camera;

    // Lambda callback registered as OnFrame (std::function).
    // The kernel wakes the libcamera thread via blocking I/O (poll on camera fd)
    // whenever a new frame has been DMA-transferred from the sensor — no polling.
    camera.registerCallback([](const cv::Mat& frame,
                               const libcamera::ControlList&) {
        {
            lock_guard<mutex> lock(frame_mutex);
            shared_frame = frame.clone();
            frame_ready  = true;
        }
        frame_cv.notify_one();
    });

    Libcam2OpenCVSettings settings;
    settings.width     = 640;
    settings.height    = 480;
    settings.framerate = 30;

    libcamera::CameraManager cm;
    cm.start();
    camera.start(cm, settings);

    namedWindow("Raw Camera",                 WINDOW_AUTOSIZE);
    namedWindow("Binary Mask (Data to Save)", WINDOW_AUTOSIZE);
    createTrackbar("Cr Min", "Binary Mask (Data to Save)", &CR_MIN, 255, on_trackbar);
    createTrackbar("Cr Max", "Binary Mask (Data to Save)", &CR_MAX, 255, on_trackbar);
    createTrackbar("Cb Min", "Binary Mask (Data to Save)", &CB_MIN, 255, on_trackbar);
    createTrackbar("Cb Max", "Binary Mask (Data to Save)", &CB_MAX, 255, on_trackbar);

    int count = 0;
    cv::Rect guideRect(100, 50, 440, 380);
    Mat frame, ycrcb, mask;

    cout << "[Target]  : " << label << endl;
    cout << "[Controls]: Adjust trackbars until background is black, hand is white." << endl;
    cout << "            's' = save mask   'q'/ESC = quit" << endl;

    while (keep_running) {
        {
            // Block here until the libcamera event thread signals a new frame.
            unique_lock<mutex> lock(frame_mutex);
            frame_cv.wait(lock, [] { return frame_ready || !keep_running; });
            if (!keep_running) break;
            frame       = shared_frame.clone();
            frame_ready = false;
        }

        Mat roi;
        if (guideRect.x + guideRect.width  <= frame.cols &&
            guideRect.y + guideRect.height <= frame.rows)
            roi = frame(guideRect).clone();
        else
            roi = frame.clone();

        cvtColor(roi, ycrcb, COLOR_BGR2YCrCb);
        inRange(ycrcb,
                Scalar(0,      CR_MIN, CB_MIN),
                Scalar(255,    CR_MAX, CB_MAX),
                mask);

        Mat kernel5 = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        Mat kernel9 = getStructuringElement(MORPH_ELLIPSE, Size(9, 9));
        dilate(mask, mask, kernel5);
        erode(mask,  mask, kernel5);
        dilate(mask, mask, kernel9);
        erode(mask,  mask, kernel9);

        cv::rectangle(frame, guideRect, cv::Scalar(0, 255, 255), 2);
        imshow("Raw Camera",                 frame);
        imshow("Binary Mask (Data to Save)", mask);

        char key = (char)waitKey(1);
        if (key == 's') {
            // Save the bounding-box-normalised mask so training data is
            // position- and scale-invariant from the start — consistent with
            // the normalisation applied in train.cpp and GestureRecognizer.
            Mat toSave = mask;
            std::vector<std::vector<cv::Point>> ctrs;
            cv::findContours(mask, ctrs, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            if (!ctrs.empty()) {
                auto maxIt = std::max_element(ctrs.begin(), ctrs.end(),
                    [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b){
                        return cv::contourArea(a) < cv::contourArea(b);
                    });
                cv::Rect bbox = cv::boundingRect(*maxIt);
                const int pad = 4;
                bbox.x      = std::max(0, bbox.x - pad);
                bbox.y      = std::max(0, bbox.y - pad);
                bbox.width  = std::min(mask.cols - bbox.x, bbox.width  + 2 * pad);
                bbox.height = std::min(mask.rows - bbox.y, bbox.height + 2 * pad);
                toSave = mask(bbox).clone();
            }
            string filename = folder + "/" + to_string(count) + ".jpg";
            imwrite(filename, toSave);
            cout << "[SAVED] " << filename << endl;
            count++;
        } else if (key == 'q' || key == 27) {
            keep_running = false;
            frame_cv.notify_all();
        }
    }

    camera.stop();
    cm.stop();
    destroyAllWindows();
    return 0;
}
