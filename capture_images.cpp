#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <sys/stat.h>

using namespace cv;
using namespace std;

int main() {
    string label = "Z"; 
    string folder = "dataset/" + label;
    system(("mkdir -p " + folder).c_str());

    // 使用最简单的设备索引打开
    VideoCapture cap(0, CAP_V4L2); 
    
    if (!cap.isOpened()) {
        cerr << "错误：无法打开摄像头。请确保排线连接正确。" << endl;
        return -1;
    }

    // 注意：不再这里写 cap.set，防止驱动冲突崩溃

    int count = 0;
    cout << "--- 取景器准备就绪 ---" << endl;
    cout << "操作：按 's' 拍照保存，按 'q' 退出" << endl;

    Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) continue;

        imshow("Collector (Press 's' to Save)", frame);

        char key = (char)waitKey(1);
        if (key == 's') {
            string filename = folder + "/" + to_string(count) + ".jpg";
            imwrite(filename, frame);
            cout << "已保存: " << filename << endl;
            count++;
        } else if (key == 'q') {
            break;
        }
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
