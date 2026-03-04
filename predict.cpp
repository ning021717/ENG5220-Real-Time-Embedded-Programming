#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>

using namespace std;
using namespace cv;
using namespace cv::ml;

int main() {
    // loading model
    Ptr<KNearest> knn = StatModel::load<KNearest>("knn_model.xml");
    if (knn.empty()) {
        cout << "❌ 错误：找不到 knn_model.xml" << endl;
        return -1;
    }
    // using GStreamer terminal：finish 640x480 resize 
    string pipeline = "libcamerasrc ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! appsink";
    
    VideoCapture cap(pipeline, CAP_GSTREAMER);

    if (!cap.isOpened()) {
        cout << "❌ GStreamer 管道启动失败，尝试普通模式..." << endl;
        cap.open(0); 
    }
// setting MJPG
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    //setting 640x480 or 320x240
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);

    HOGDescriptor hog(Size(64, 64), Size(16, 16), Size(8, 8), Size(8, 8), 9);
    char last_char = ' ';
    int stable_count = 0;

    cout << "🚀 识别系统已启动！按 ESC 退出。" << endl;

    while (true) {
        Mat frame, gray, res;
        cap >> frame;
        if (frame.empty()) continue;

        flip(frame, frame, 1);
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        resize(gray, res, Size(64, 64));

        vector<float> descriptors;
        hog.compute(res, descriptors);
        
        float result = knn->predict(Mat(descriptors).t());
        char current_char = 'A' + (int)result;

        // diagram result
        string text = "Gesture: ";
        text += current_char;
        putText(frame, text, Point(20, 60), FONT_HERSHEY_SIMPLEX, 1.2, Scalar(0, 255, 0), 2);
        
        imshow("Hand Sign Recognition", frame);

        // report stability
        if (current_char == last_char) {
            stable_count++;
            if (stable_count == 15) { //It will only be broadcast if 15 consecutive frames are identical.
                cout << ">>> 识别到: " << current_char << endl;
                string cmd = "espeak '" + string(1, current_char) + "' &";
                system(cmd.c_str());
                stable_count = 0;
            }
        } else {
            stable_count = 0;
            last_char = current_char;
        }

        if (waitKey(1) == 27) break;
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
