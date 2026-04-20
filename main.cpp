#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>

#include "CameraManager.hpp"
#include "GestureRecognizer.hpp"
#include "VoiceSynthesizer.hpp"

using namespace cv;
using namespace std;

// ==========================================
// IPC & Thread Synchronisation
// ==========================================
mutex              mtx;
condition_variable cv_frame_ready;
Mat                shared_roi;
bool               frame_ready = false;
atomic<bool>       keep_running(true);

const string WINDOW_MASK = "Binary Mask";

void on_trackbar(int, void*) {}

// ==========================================
// EVENT CALLBACK: called from libcamera event thread
// The kernel unblocks that thread via blocking I/O on the camera fd;
// this callback then wakes the consumer (main) thread via condition_variable.
// ==========================================
void onFrameCaptured(const cv::Mat& roi) {
    {
        lock_guard<mutex> lock(mtx);
        shared_roi  = roi.clone();
        frame_ready = true;
    }
    cv_frame_ready.notify_one();
}

int main() {
    // ==========================================
    // SIGNAL HANDLING via signalfd (blocking I/O)
    //
    // Rather than registering an async-signal-unsafe signal() callback,
    // we block SIGINT/SIGTERM at the process level and create a file
    // descriptor (sfd) that becomes readable when one of those signals
    // arrives. A dedicated thread blocks on read(sfd) — this is the same
    // "blocking I/O wakes up threads" principle used for the camera.
    // ==========================================
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGINT);
    sigaddset(&sig_mask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &sig_mask, nullptr) == -1) {
        perror("sigprocmask");
        return -1;
    }

    int sfd = signalfd(-1, &sig_mask, SFD_CLOEXEC);
    if (sfd == -1) {
        perror("signalfd");
        return -1;
    }

    // Signal-watcher thread: blocks on read(sfd) until Ctrl-C or SIGTERM.
    // read() returns only when the kernel delivers a signal — zero polling.
    thread sigThread([sfd]() {
        struct signalfd_siginfo fdsi;
        ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
        if (s == sizeof(fdsi))
            cout << "\n[INTERRUPT] Signal " << fdsi.ssi_signo
                 << " received. Shutting down gracefully..." << endl;
        keep_running = false;
        cv_frame_ready.notify_all();
        close(sfd);
    });
    sigThread.detach();

    // ==========================================
    // 1. CAMERA
    // ==========================================
    cout << "[INFO] Initializing Camera Pipeline..." << endl;
    CameraManager cam(0);

    // ==========================================
    // 2. AI ENGINE & TTS
    // ==========================================
    // Prefer the CNN ONNX model when present — it is position/scale invariant
    // and significantly more accurate than raw-pixel KNN.  Fall back to KNN if
    // gesture_cnn.onnx has not been placed in the project root.
    const string cnnPath = "gesture_cnn.onnx";
    const string knnPath = "knn_model.xml";
    const string modelPath = std::filesystem::exists(cnnPath) ? cnnPath : knnPath;
    cout << "[INFO] Loading AI Engine (" << modelPath << ")..." << endl;
    GestureRecognizer recognizer(modelPath);
    if (!recognizer.isModelLoaded()) return -1;
    cout << "[INFO] Backend: " << recognizer.backendName() << endl;

    cout << "[INFO] Loading Voice Synthesizer..." << endl;
    VoiceSynthesizer voice;

    string lastSpokenText = "";
    int    framesConfirmed = 0;
    const int CONFIRMATION_THRESHOLD = 6;

    // ==========================================
    // 3. GUI WINDOWS
    // ==========================================
    const string windowCapture =
        string("Sign Language Translator [") + recognizer.backendName() + "]";
    namedWindow(windowCapture);
    namedWindow(WINDOW_MASK);
    createTrackbar("Cr Min", windowCapture, &recognizer.CR_MIN, 255, on_trackbar);
    createTrackbar("Cr Max", windowCapture, &recognizer.CR_MAX, 255, on_trackbar);
    createTrackbar("Cb Min", windowCapture, &recognizer.CB_MIN, 255, on_trackbar);
    createTrackbar("Cb Max", windowCapture, &recognizer.CB_MAX, 255, on_trackbar);

    cam.startCapture(onFrameCaptured);
    cout << "[INFO] Real-Time System Online (Press ESC or Ctrl+C to quit)" << endl;

    // ==========================================
    // CONSUMER LOOP (Event-Driven)
    // Blocks on condition_variable::wait() — woken only by a hardware frame
    // event from the libcamera callback, never by a timer or busy-poll.
    // ==========================================
    Mat local_roi, mask;
    while (keep_running) {
        {
            unique_lock<mutex> lock(mtx);
            cv_frame_ready.wait(lock, [] { return frame_ready || !keep_running; });
            if (!keep_running) break;
            local_roi   = shared_roi.clone();
            frame_ready = false;
        }

        // --- Inference ---
        string text = recognizer.predict(local_roi, mask);

        // --- Anti-spam debounce: speak only after CONFIRMATION_THRESHOLD stable frames ---
        if (text != "No Hand" && text != "Error" && text != "Uncertain") {
            if (text == lastSpokenText) {
                framesConfirmed++;
                if (framesConfirmed == CONFIRMATION_THRESHOLD)
                    voice.speak(text);
            } else {
                lastSpokenText  = text;
                framesConfirmed = 0;
            }
        } else {
            framesConfirmed = 0;
            lastSpokenText  = "";
        }

        // --- GUI ---
        Mat display_frame = local_roi.clone();
        if (text != "No Hand" && text != "Error" && text != "Uncertain")
            putText(display_frame, "Detected: " + text,
                    Point(10, 40), FONT_HERSHEY_SIMPLEX, 1.2, Scalar(0, 255, 0), 3);
        else if (text == "Uncertain")
            putText(display_frame, "...",
                    Point(10, 40), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 165, 255), 2);
        else
            putText(display_frame, text,
                    Point(10, 40), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255), 2);

        imshow(windowCapture, display_frame);
        if (!mask.empty()) imshow(WINDOW_MASK, mask);

        if ((char)waitKey(1) == 27) { // ESC
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
