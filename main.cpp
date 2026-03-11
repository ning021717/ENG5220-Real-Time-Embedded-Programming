/**
 * @file main.cpp
 * @brief Real-time BSL Translator with KNN and Convexity Defects correction.
 */

#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm> // For replace()

using namespace cv;
using namespace cv::ml;
using namespace std;

// Skin color thresholds (Adjust via Trackbars if necessary)
int H_MIN = 0, H_MAX = 20;
int S_MIN = 30, S_MAX = 255;
int V_MIN = 30, V_MAX = 255;

const int IMG_SIZE = 50; 
string last_spoken_text = ""; 
int stable_frames = 0;        

// Function to invoke the TTS engine (espeak) in the background
void speak(string text) {
    string safe_text = text;
    // Replace spaces with underscores to prevent shell command errors
    replace(safe_text.begin(), safe_text.end(), ' ', '_');
    string command = "espeak \"" + safe_text + "\" &"; 
    system(command.c_str()); 
}

// Map KNN class labels to BSL text
string getLabelText(float label) {
    int i = (int)label;
    if (i == 0) return "Yes";
    if (i == 1) return "No";
    if (i == 2) return "Thank you";
    if (i == 3) return "I love you";
    return "Unknown";
}

void on_trackbar(int, void*) {}

int main() {
    // 1. Load the trained KNN model
    Ptr<KNearest> knn = KNearest::load("knn_model.xml");
    if (knn.empty()) {
        cerr << "Error: knn_model.xml not found!" << endl;
        return -1;
    }

    // 2. Initialize camera (using V4L2 for Raspberry Pi)
    VideoCapture cap(0, CAP_V4L2);
    if (!cap.isOpened()) return -1;
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);

    namedWindow("BSL Translator", WINDOW_AUTOSIZE);
    namedWindow("Mask", WINDOW_AUTOSIZE);
    
    // Large ROI (Region of Interest) suitable for BSL gestures
    Rect roiRect(150, 80, 340, 340); 
    Mat frame, roi, hsv, mask, processingImg;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        flip(frame, frame, 1); // Mirror effect
        
        roi = frame(roiRect);
        cvtColor(roi, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), mask);
        
        // Morphological operations to remove noise
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        erode(mask, mask, kernel);
        dilate(mask, mask, kernel);

        string current_text = "No Hand";
        int finger_gaps = 0;

        // 3. Find contours to check hand size and calculate defects
        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        if (!contours.empty()) {
            int max_idx = 0;
            double max_area = 0;
            // Find the largest contour (the hand)
            for(size_t i = 0; i < contours.size(); i++) {
                double area = contourArea(contours[i]);
                if(area > max_area) { max_area = area; max_idx = i; }
            }

            // Only process if a significant hand shape is detected
            if (max_area > 1500) {
                
                // --- A. CONVEXITY DEFECTS LOGIC ---
                vector<int> hull_ints;
                convexHull(contours[max_idx], hull_ints, false);
                vector<Vec4i> defects;
                
                if (hull_ints.size() > 3) {
                    convexityDefects(contours[max_idx], hull_ints, defects);
                    for (const Vec4i& v : defects) {
                        float depth = v[3] / 256.0f;
                        if (depth > 20.0) { // Deep gaps mean extended fingers
                            finger_gaps++;
                        }
                    }
                }

                // --- B. KNN PREDICTION LOGIC ---
                mask.copyTo(processingImg);
                resize(processingImg, processingImg, Size(IMG_SIZE, IMG_SIZE));
                processingImg = processingImg.reshape(1, 1);
                processingImg.convertTo(processingImg, CV_32F);

                float result = knn->findNearest(processingImg, 5, noArray());
                current_text = getLabelText(result);

                // --- C. THE GEEK CORRECTION (Decision Tree) ---
                // Conflict resolution: Spider-Man gesture vs Fist
                if (current_text == "Yes" && finger_gaps >= 2) {
                    current_text = "I love you"; 
                } else if (current_text == "I love you" && finger_gaps == 0) {
                    current_text = "Yes";
                }

                rectangle(frame, roiRect, Scalar(0, 255, 0), 2);
            } else {
                rectangle(frame, roiRect, Scalar(0, 0, 255), 2);
            }
        } else {
            rectangle(frame, roiRect, Scalar(0, 0, 255), 2);
        }

        // --- 4. TTS Debounce Logic (Prevent repetitive speaking) ---
        if (current_text != "No Hand" && current_text != "Unknown") {
            if (current_text == last_spoken_text) {
                stable_frames = 0; 
            } else {
                static string temp_text = "";
                static int debounce_counter = 0;

                if (current_text != temp_text) {
                    temp_text = current_text;
                    debounce_counter = 0;
                } else {
                    debounce_counter++;
                }

                if (debounce_counter > 8) { // Wait for stable gesture
                    cout << "BSL Detected: " << current_text << " (Gaps: " << finger_gaps << ")" << endl;
                    speak(current_text);
                    last_spoken_text = current_text; 
                    debounce_counter = 0;
                }
            }
        } else {
            last_spoken_text = "";
        }

        // 5. Draw UI
        putText(frame, current_text, Point(160, 70), FONT_HERSHEY_SIMPLEX, 1.2, Scalar(255, 0, 0), 3);
        putText(frame, "Gaps: " + to_string(finger_gaps), Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 0, 255), 2);
        
        imshow("BSL Translator", frame);
        imshow("Mask", mask);

        if (waitKey(30) == 27) break; // ESC to exit
    }
    cap.release(); destroyAllWindows(); return 0;
}