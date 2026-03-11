/**
 * @file defects_test.cpp
 * @brief Hand gesture differentiation using Convexity Defects (Pure C++ / OpenCV)
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

// Skin color thresholds (Adjust if necessary)
int H_MIN = 0, H_MAX = 20;
int S_MIN = 30, S_MAX = 255;
int V_MIN = 30, V_MAX = 255;

void on_trackbar(int, void*) {}

int main() {
    VideoCapture cap(0, CAP_V4L2);
    if (!cap.isOpened()) return -1;
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);

    namedWindow("Defects Tracker", WINDOW_AUTOSIZE);
    namedWindow("Mask", WINDOW_AUTOSIZE);
    namedWindow("Trackbars", WINDOW_AUTOSIZE);

    createTrackbar("H Min", "Trackbars", &H_MIN, 179, on_trackbar);
    createTrackbar("H Max", "Trackbars", &H_MAX, 179, on_trackbar);
    createTrackbar("S Min", "Trackbars", &S_MIN, 255, on_trackbar);
    createTrackbar("S Max", "Trackbars", &S_MAX, 255, on_trackbar);

    Rect roiRect(150, 80, 340, 340);
    Mat frame, roi, hsv, mask;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        flip(frame, frame, 1);
        
        roi = frame(roiRect);
        cvtColor(roi, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), mask);
        
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        erode(mask, mask, kernel);
        dilate(mask, mask, kernel);

        // 1. Find all contours in the binary mask
        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        if (!contours.empty()) {
            // 2. Find the largest contour (assuming it's the hand)
            int max_idx = 0;
            double max_area = 0;
            for(int i = 0; i < contours.size(); i++) {
                double area = contourArea(contours[i]);
                if(area > max_area) { max_area = area; max_idx = i; }
            }

            // Only process if the hand is big enough
            if (max_area > 2000) {
                vector<Point> hull_points;
                vector<int> hull_ints;
                
                // 3. Calculate Convex Hull (Outer boundary)
                convexHull(contours[max_idx], hull_points, true);
                convexHull(contours[max_idx], hull_ints, false);

                // Draw the contour (Yellow) and the Hull (Blue)
                drawContours(roi, contours, max_idx, Scalar(0, 255, 255), 2);
                polylines(roi, hull_points, true, Scalar(255, 0, 0), 2);

                // 4. Calculate Convexity Defects (Gaps between fingers)
                vector<Vec4i> defects;
                int finger_gaps = 0;
                
                if (hull_ints.size() > 3) {
                    convexityDefects(contours[max_idx], hull_ints, defects);
                    
                    for (const Vec4i& v : defects) {
                        // v[3] is the depth of the defect
                        float depth = v[3] / 256.0f;
                        
                        // Filter out shallow defects (noise), only count deep gaps
                        if (depth > 20.0) {
                            finger_gaps++;
                            Point start = contours[max_idx][v[0]];
                            Point end = contours[max_idx][v[1]];
                            Point far = contours[max_idx][v[2]]; // The deepest point of the gap
                            
                            // Draw a red circle at the deep gap
                            circle(roi, far, 6, Scalar(0, 0, 255), -1);
                        }
                    }
                }
                
                // Display the number of gaps detected
                putText(frame, "Finger Gaps: " + to_string(finger_gaps), Point(10, 30), 
                        FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 0, 255), 3);
            }
        }

        rectangle(frame, roiRect, Scalar(0, 255, 0), 2);
        imshow("Defects Tracker", frame);
        imshow("Mask", mask);

        if (waitKey(30) == 27) break;
    }
    cap.release(); destroyAllWindows(); return 0;
}
