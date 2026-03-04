#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp> // A machine learning module must be introduced.
#include <iostream>
#include <vector>
#include <string>

using namespace cv;
using namespace cv::ml; // Using machine learning namespaces
using namespace std;

// Skin color threshold 
int H_MIN = 0;
int H_MAX = 20;
int S_MIN = 30;
int S_MAX = 255;
int V_MIN = 30;
int V_MAX = 255;

const string WINDOW_CAPTURE = "Sign Language Translator";
const string WINDOW_MASK = "Binary Mask";
const string WINDOW_TRACKBAR = "Settings";

// It must be consistent with the training!
const int IMG_SIZE = 50; 

void on_trackbar(int, void*) {}

// Auxiliary function: Convert the numbers 0, 1, 2 into the letters 'A', 'B', 'C'.
string getLabelText(float label) {
    int i = (int)label;
    if (i == 0) return "A";
    if (i == 1) return "B";
    if (i == 2) return "C";
    return "Unknown";
}

int main() {
    // 1. loading model
    cout << "正在加载模型..." << endl;
    Ptr<KNearest> knn = KNearest::load("knn_model.xml"); // 确保文件名对

    if (knn.empty()) {
        cerr << "错误：找不到 knn_model.xml！请先运行 TrainModel。" << endl;
        return -1;
    }
    cout << "模型加载成功！" << endl;

    // 2. camera on 
    VideoCapture cap(0, CAP_V4L2);
    if (!cap.isOpened()) {
        cerr << "错误：无法打开摄像头" << endl;
        return -1;
    }

    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);

    namedWindow(WINDOW_CAPTURE);
    namedWindow(WINDOW_MASK);
    namedWindow(WINDOW_TRACKBAR);

    createTrackbar("H Min", WINDOW_TRACKBAR, &H_MIN, 179, on_trackbar);
    createTrackbar("H Max", WINDOW_TRACKBAR, &H_MAX, 179, on_trackbar);
    createTrackbar("S Min", WINDOW_TRACKBAR, &S_MIN, 255, on_trackbar);
    createTrackbar("S Max", WINDOW_TRACKBAR, &S_MAX, 255, on_trackbar);
    createTrackbar("V Min", WINDOW_TRACKBAR, &V_MIN, 255, on_trackbar);
    createTrackbar("V Max", WINDOW_TRACKBAR, &V_MAX, 255, on_trackbar);

    Mat frame, roi, hsv, mask, processingImg;
    Rect roiRect(350, 50, 250, 250); 

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        flip(frame, frame, 1);
        roi = frame(roiRect);

        cvtColor(roi, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), mask);

        // noisy remove
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        erode(mask, mask, kernel);
        dilate(mask, mask, kernel);

        // logical
        // 1. Make a copy of the mask for processing to avoid damaging the image used for display.
        mask.copyTo(processingImg);

        // 2. switching size
        resize(processingImg, processingImg, Size(IMG_SIZE, IMG_SIZE));

        // 3. Flat 
        processingImg = processingImg.reshape(1, 1);

        // 4. floating
        processingImg.convertTo(processingImg, CV_32F);

        // 5. forecast
        float result = knn->findNearest(processingImg, 5, noArray()); // K=5

        // 6. getting text
        string text = getLabelText(result);

        // result
        // show on ROI
        // The image is only recognized if there are white pixels in the mask (to avoid recognizing a completely black background as an A).
        if (countNonZero(mask) > 1000) {
            rectangle(frame, roiRect, Scalar(0, 255, 0), 2); // Green box: Recognizing
            putText(frame, "Detected: " + text, Point(350, 40), FONT_HERSHEY_SIMPLEX, 1.2, Scalar(0, 255, 0), 3);
        } else {
            rectangle(frame, roiRect, Scalar(0, 0, 255), 2); // Red box: No hand detected
            putText(frame, "No Hand", Point(350, 40), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255), 2);
        }

        imshow(WINDOW_CAPTURE, frame);
        imshow(WINDOW_MASK, mask);

        char c = (char)waitKey(30);
        if (c == 27) break; // ESC 退出
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
