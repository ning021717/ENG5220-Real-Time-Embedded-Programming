#include "CameraManager.hpp"
#include "GestureRecognizer.hpp"
#include "Translator.hpp"

int main() {
    CameraManager camera(0);
    GestureRecognizer recognizer;
    Translator translator;

    if (!camera.init()) return -1;

    // ROI 区域定义
    cv::Rect handBox(100, 100, 300, 300);

    // 注册 Callback 链路
    camera.startCapture([&](const cv::Mat& frame) {
        cv::Mat roi = frame(handBox).clone();

        // 识别模块处理
        recognizer.process(roi, [&](std::string result) {
            // 翻译模块输出
            translator.translate(result);
        });

        // 渲染界面
        cv::rectangle(frame, handBox, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Sign Language System", frame);
    });

    while (cv::waitKey(1) != 27) {
        // 事件循环，保持程序响应
    }
    camera.stop();
    return 0;
}