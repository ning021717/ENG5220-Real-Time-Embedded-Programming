#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable> 
#include <atomic>
#include <csignal>
#include <unistd.h>           

#include "CameraManager.hpp"  
#include "GestureRecognizer.hpp" // [NEW] Our encapsulated AI engine
#include "VoiceSynthesizer.hpp" // [NEW] Include our Audio Engine

using namespace cv;
using namespace std;

// ==========================================
// IPC & Thread Synchronization Tools
// ==========================================
mutex mtx;
condition_variable cv_frame_ready;
Mat shared_roi;
bool frame_ready = false;
atomic<bool> keep_running(true);

const string WINDOW_CAPTURE = "Sign Language Translator";
const string WINDOW_MASK = "Binary Mask";

// Dummy callback for trackbars
void on_trackbar(int, void*) {}

// ==========================================
// SIGINT (Ctrl+C) Interceptor with "Double-Tap" Force Quit
// ==========================================
void signalHandler(int signum) {
    static int sig_count = 0;
    sig_count++;
    
    if (sig_count >= 2) {
        cout << "\n[FATAL] Multiple interrupts detected. Force quitting immediately!" << endl;
        _exit(1); 
    }
    
    cout << "\n[INTERRUPT] Signal (" << signum << ") received. Shutting down gracefully..." << endl;
    keep_running = false;            
    cv_frame_ready.notify_all(); 
}

// ==========================================
// EVENT CALLBACK: Triggered by CameraManager
// ==========================================
void onFrameCaptured(const cv::Mat& roi) {
    {
        lock_guard<mutex> lock(mtx);
        shared_roi = roi.clone();
        frame_ready = true;
    }
    cv_frame_ready.notify_one(); 
}

int main() {
    signal(SIGINT, signalHandler);

    // 1. INITIALIZE CAMERA FIRST
    // libcamera is initialised inside startCapture() via Libcam2OpenCV::start().
    // No separate init() call needed — the library validates the camera there.
    cout << "[INFO] Initializing Camera Pipeline..." << endl;
    CameraManager cam(0);

    // 2. INITIALIZE AI ENGINE (Encapsulated OOP)
    cout << "[INFO] Loading AI Engine..." << endl;
    GestureRecognizer recognizer("knn_model.xml");
    if (!recognizer.isModelLoaded()) return -1;
    
    cout << "[INFO] Loading Voice Synthesizer..." << endl;
    VoiceSynthesizer voice; 

    // Variables for Anti-Spam Logic
    string lastSpokenText = "";
    int framesConfirmed = 0;
    const int CONFIRMATION_THRESHOLD = 10; // Must see same gesture for 10 frames
    
    // 3. CREATE GUI WINDOWS & BIND TRACKBARS TO OBJECT
    namedWindow(WINDOW_CAPTURE);
    namedWindow(WINDOW_MASK);
    createTrackbar("H Min", WINDOW_CAPTURE, &recognizer.H_MIN, 179, on_trackbar);
    createTrackbar("H Max", WINDOW_CAPTURE, &recognizer.H_MAX, 179, on_trackbar);
    createTrackbar("S Min", WINDOW_CAPTURE, &recognizer.S_MIN, 255, on_trackbar);
    createTrackbar("S Max", WINDOW_CAPTURE, &recognizer.S_MAX, 255, on_trackbar);

    cam.startCapture(onFrameCaptured);

    Mat local_roi, mask;
    cout << "[INFO] Real-Time System Online (Press ESC or Ctrl+C to quit)" << endl;

    // ==========================================
    // CONSUMER THREAD (Event-Driven)
    // ==========================================
    while (keep_running) {
        {
            unique_lock<mutex> lock(mtx);
            cv_frame_ready.wait(lock, []{ return frame_ready || !keep_running; });
            if (!keep_running) break;
            local_roi = shared_roi.clone();
            frame_ready = false; 
        }

        // --- Core Algorithm ---
        string text = recognizer.predict(local_roi, mask);

        // --- Anti-Spam & Voice Logic ---
        if (text != "No Hand" && text != "Error") {
            if (text == lastSpokenText) {
                framesConfirmed++;
                // If the gesture is stable for 10 consecutive frames, speak it!
                if (framesConfirmed == CONFIRMATION_THRESHOLD) {
                    voice.speak(text);
                }
            } else {
                // Gesture changed, reset counter
                lastSpokenText = text;
                framesConfirmed = 0;
            }
        } else {
            // Hand lost, reset counter
            framesConfirmed = 0;
            lastSpokenText = "";
        }

        // --- GUI Rendering ---
        Mat display_frame = local_roi.clone();
        if (text != "No Hand" && text != "Error") {
            putText(display_frame, "Detected: " + text, Point(10, 40), FONT_HERSHEY_SIMPLEX, 1.2, Scalar(0, 255, 0), 3);
        } else {
            putText(display_frame, text, Point(10, 40), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255), 2);
        }

        imshow(WINDOW_CAPTURE, display_frame);
        if (!mask.empty()) imshow(WINDOW_MASK, mask);

        char c = (char)waitKey(1);
        if (c == 27) { // ESC key
            keep_running = false;
            cv_frame_ready.notify_all(); 
            break;
        }
    }

    cam.stop();
    destroyAllWindows();
    cout << "[INFO] System shut down successfully." << endl;
    return 0;
}
