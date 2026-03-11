/**
 * @file tflite_test.cpp
 * @brief Hand skeleton tracking using OpenCV DNN and MediaPipe TFLite model.
 */

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>

using namespace cv;
using namespace cv::dnn;
using namespace std;

int main() {
    // 1. Load the TFLite model via OpenCV DNN module
    cout << "Loading TFLite model via OpenCV DNN..." << endl;
    Net net;
    try {
        net = readNetFromTFLite("hand_landmark.tflite");
    } catch (const Exception& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << "Please ensure 'hand_landmark.tflite' is in the current directory." << endl;
        return -1;
    }
    cout << "Model loaded successfully!" << endl;

    // 2. Initialize the camera (using V4L2 for Raspberry Pi compatibility)
    VideoCapture cap(0, CAP_V4L2);
    if (!cap.isOpened()) {
        cerr << "Error: Cannot open the camera." << endl;
        return -1;
    }
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);

    namedWindow("TFLite Skeleton Tracker", WINDOW_AUTOSIZE);

    // Define the Region of Interest (ROI) for the hand
    // Using a large bounding box (340x340) for better capture
    Rect roiRect(150, 80, 340, 340);
    Mat frame, roi, blob;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        flip(frame, frame, 1); // Mirror the frame
        
        // Draw the bounding box on the original frame
        rectangle(frame, roiRect, Scalar(0, 255, 0), 2);
        
        // Extract the colored ROI
        roi = frame(roiRect);

        // 3. Preprocess the image (Blob conversion)
        // The MediaPipe model requires 256x256 input, normalized to [0, 1]
        blobFromImage(roi, blob, 1.0 / 255.0, Size(256, 256), Scalar(0, 0, 0), true, false);

        // 4. Set the input and run the forward pass
        net.setInput(blob);
        vector<Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        // 5. Parse the output (21 landmarks * 3 coordinates [x,y,z] = 63 floats)
        if (!outputs.empty()) {
            Mat landmarks = outputs[0]; 
            if (landmarks.total() >= 63) {
                float* data = (float*)landmarks.data;
                
                // Draw the 21 landmarks on the hand
                for (int i = 0; i < 21; i++) {
                    // The model outputs normalized coordinates relative to the 256x256 input
                    float x = data[i * 3] / 256.0 * roiRect.width;
                    float y = data[i * 3 + 1] / 256.0 * roiRect.height;
                    
                    // Map coordinates back to the original frame (adding ROI offset)
                    Point pt(roiRect.x + x, roiRect.y + y);
                    
                    // Draw a red circle for each joint landmark
                    circle(frame, pt, 4, Scalar(0, 0, 255), -1);
                }
                putText(frame, "Skeleton Tracking Active", Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
            }
        }

        imshow("TFLite Skeleton Tracker", frame);

        // Exit loop if ESC key is pressed
        if (waitKey(30) == 27) break; 
    }

    // Clean up
    cap.release();
    destroyAllWindows();
    return 0;
}
