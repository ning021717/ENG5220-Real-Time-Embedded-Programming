#include "CameraManager.hpp"
#include "GestureRecognizer.hpp"
#include "SignDatabase.hpp"
#include "Translator.hpp"
#include <iostream>

int main() {
    // 1. 初始化并加载数据库
    SignDatabase db;
    // ⚠️ 注意：这里使用了你截图中的路径，Windows下路径要用双斜杠
    std::string dbPath = "E:\\gesture_bin_files";

    std::cout << "正在加载手语库..." << std::endl;
    if (!db.loadFromFolder(dbPath)) {
        std::cerr << "无法加载手语库！请检查路径。" << std::endl;
        return -1;
    }

    // 2. 初始化模块
    CameraManager camera(0);
    GestureRecognizer recognizer;
    Translator translator;

    if (!camera.init()) {
        std::cerr << "摄像头启动失败" << std::endl;
        return -1;
    }

    // 3. 启动采集与识别循环
    std::cout << "系统启动！按 ESC 退出。" << std::endl;

    camera.startCapture([&](const cv::Mat& frame) {
        // 将 frame 和 db 传给识别器
        recognizer.process(frame, db, [&](const std::string& result) {
            // 识别成功，调用翻译模块
            translator.translate(result);
        });
    });

    // 4. 等待退出 (实时性修正版)
    while (true) {
        if (cv::waitKey(10) == 27) break;
    }

    camera.stop();
    return 0;
}