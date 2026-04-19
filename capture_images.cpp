#include "libcam2opencv.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <sys/stat.h>
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

int H_MIN = 0,  H_MAX = 20;
int S_MIN = 30, S_MAX = 255;
int V_MIN = 30, V_MAX = 255;

void on_trackbar(int, void*) {}

// ==========================================
// Libcamera callback — called from the libcamera event thread.
// The kernel wakes that thread via blocking I/O (poll on the camera fd)
// whenever a new frame has been DMA-transferred from the sensor. This is
// NOT a polling loop: the OS itself drives execution here.
// ==========================================
struct CaptureCallback : public Libcam2OpenCV::Callback {
    void hasFrame(const cv::Mat& frame,
                  const libcamera::ControlList&) override {
        {
            lock_guard<mutex> lock(frame_mutex);
            shared_frame = frame.clone();
            frame_ready  = true;
        }
        // Wake the GUI thread that is blocked on condition_variable::wait().
        // This is the "blocking I/O wakes up threads" pattern: the GUI thread
        // sleeps until here, and here is reached only on a hardware event.
        frame_cv.notify_one();
    }
};

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
    system(("mkdir -p " + folder).c_str());

    CaptureCallback    captureCallback;
    Libcam2OpenCV      camera;
    camera.registerCallback(&captureCallback);

    Libcam2OpenCVSettings settings;
    settings.width     = 640;
    settings.height    = 480;
    settings.framerate = 30;
    camera.start(settings);

    namedWindow("Raw Camera",                 WINDOW_AUTOSIZE);
    namedWindow("Binary Mask (Data to Save)", WINDOW_AUTOSIZE);
    createTrackbar("H Min", "Binary Mask (Data to Save)", &H_MIN, 179, on_trackbar);
    createTrackbar("H Max", "Binary Mask (Data to Save)", &H_MAX, 179, on_trackbar);
    createTrackbar("S Min", "Binary Mask (Data to Save)", &S_MIN, 255, on_trackbar);
    createTrackbar("S Max", "Binary Mask (Data to Save)", &S_MAX, 255, on_trackbar);

    int count = 0;
    cv::Rect guideRect(100, 50, 440, 380);
    Mat frame, hsv, mask;

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

        cvtColor(roi, hsv, COLOR_BGR2HSV);
        inRange(hsv,
                Scalar(H_MIN, S_MIN, V_MIN),
                Scalar(H_MAX, S_MAX, V_MAX),
                mask);

        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        erode(mask,  mask, kernel);
        dilate(mask, mask, kernel);

        cv::rectangle(frame, guideRect, cv::Scalar(0, 255, 255), 2);
        imshow("Raw Camera",                 frame);
        imshow("Binary Mask (Data to Save)", mask);

        char key = (char)waitKey(1);
        if (key == 's') {
            string filename = folder + "/" + to_string(count) + ".jpg";
            imwrite(filename, mask);
            cout << "[SAVED] " << filename << endl;
            count++;
        } else if (key == 'q' || key == 27) {
            keep_running = false;
            frame_cv.notify_all();
        }
    }

    camera.stop();
    destroyAllWindows();
    return 0;
}
