#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <cmath> // 确保 sqrt/pow 有定义
#include <thread>
#include <mutex>

using namespace std;
using namespace cv;

// 互斥锁，用于线程同步
mutex mtx;

// 回调函数，用于处理每一帧
void processFrame(Mat frame, vector<vector<Point>> contours, Rect hand_roi, Scalar lower_skin, Scalar upper_skin) {
    Mat hsv_frame, mask;
    vector<Vec4i> hierarchy;

    // 转换为 HSV 色彩空间并应用肤色掩码
    cvtColor(frame, hsv_frame, COLOR_BGR2HSV);
    inRange(hsv_frame, lower_skin, upper_skin, mask);

    // 降噪（先闭运算，再开运算）
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(7, 7));
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);
    morphologyEx(mask, mask, MORPH_OPEN, kernel);

    // 查找轮廓
    findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (!contours.empty()) {
        // 处理最大轮廓
        int max_contour_idx = 0;
        double max_area = 0;
        for (int i = 0; i < contours.size(); i++) {
            double area = contourArea(contours[i]);
            if (area > max_area && area > 2000) { // 过滤掉小轮廓
                max_area = area;
                max_contour_idx = i;
            }
        }

        // 绘制轮廓
        drawContours(frame, contours, max_contour_idx, Scalar(0, 255, 0), 2);

        // 通过矩计算轮廓的中心点
        Moments m = moments(contours[max_contour_idx]);
        if (m.m00 != 0) {
            Point center(static_cast<int>(m.m10 / m.m00), static_cast<int>(m.m01 / m.m00));
            circle(frame, center, 5, Scalar(255, 255, 0), -1);
        }

        // 手势检测
        int finger_count = 0;
        vector<Vec4i> defects;
        convexityDefects(contours[max_contour_idx], contours[max_contour_idx], defects);

        // 基于凸缺陷检测手指
        for (Vec4i d : defects) {
            int start_idx = d[0];
            int end_idx = d[1];
            int far_idx = d[2];
            double depth = d[3] / 256.0;

            if (depth > 30) {
                finger_count++;
            }
        }

        // 显示识别到的手指数量
        putText(frame, "手指数量: " + to_string(finger_count), Point(20, 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 3);
    }

    // 显示当前帧
    imshow("手势识别", frame);
}

// 捕获视频帧的线程函数
void captureFrames(VideoCapture &cap, Rect hand_roi, Scalar lower_skin, Scalar upper_skin) {
    Mat frame;
    vector<vector<Point>> contours;

    while (true) {
        bool ret = cap.read(frame);
        if (!ret || frame.empty()) {
            cout << "⚠️ 无法捕获帧，正在重试..." << endl;
            continue;
        }

        // 镜像翻转帧（符合直觉）
        flip(frame, frame, 1);

        // 为每一帧创建一个单独的线程进行处理
        thread process_thread(processFrame, frame, contours, hand_roi, lower_skin, upper_skin);
        process_thread.detach(); // 异步处理

        int key = waitKey(10) & 0xFF;
        if (key == 'q' || key == 27) { // 按 'q' 或 ESC 退出
            cout << "程序正在退出..." << endl;
            break;
        } else if (key == 's') { // 按 's' 保存当前帧
            imwrite("hand_gesture.jpg", frame);
            cout << "已保存当前帧为 hand_gesture.jpg" << endl;
        }
    }
}

int main() {
    // 设置控制台为 UTF-8 编码，避免中文乱码
    SetConsoleOutputCP(CP_UTF8);

    // 打开摄像头
    cout << "🔍 正在检测摄像头..." << endl;
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cout << "⚠️ 未能找到摄像头，尝试连接树莓派 Camera Module 2..." << endl;
        cap.open("/dev/video0"); // 树莓派 Camera Module 2 使用的设备路径
        if (!cap.isOpened()) {
            cout << "❌ 无法找到摄像头!" << endl;
            return -1;
        }
    }

    // 设置摄像头参数（分辨率和帧率）
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FPS, 30);

    cout << "✅ 摄像头打开成功！" << endl;

    // 定义手部检测区域
    Rect hand_roi = Rect(100, 50, 440, 380);

    // 定义肤色检测的 HSV 范围
    Scalar lower_skin = Scalar(5, 30, 80);  // 调整为适应不同肤色
    Scalar upper_skin = Scalar(30, 255, 255);

    // 启动视频捕获线程
    thread capture_thread(captureFrames, ref(cap), hand_roi, lower_skin, upper_skin);
    capture_thread.join(); // 等待捕获线程结束

    cap.release();
    destroyAllWindows();
    return 0;
}
