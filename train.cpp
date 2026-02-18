#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp> // 引入机器学习模块
#include <iostream>
#include <vector>
#include <string>

using namespace cv;
using namespace cv::ml;
using namespace std;

// 图片将被压缩到这个尺寸进行训练（太大了算得慢，太小了看不清）
const int IMG_SIZE = 50; 

// 读取文件夹内的所有图片路径
void load_images(string directory, int label, Mat& trainData, Mat& trainLabels) {
    vector<String> filenames;
    // 使用 OpenCV 自带的 glob 函数读取文件名
    glob(directory + "/*.jpg", filenames); 

    if(filenames.empty()) {
        cout << "警告: 文件夹 " << directory << " 是空的！" << endl;
        return;
    }

    cout << "正在处理 " << directory << " (" << filenames.size() << " 张图片)..." << endl;

    for (size_t i = 0; i < filenames.size(); ++i) {
        Mat img = imread(filenames[i], IMREAD_GRAYSCALE);
        
        if (img.empty()) continue;

        // 1. 调整大小 (Resize) 到固定尺寸 (50x50)
        resize(img, img, Size(IMG_SIZE, IMG_SIZE));

        // 2. 扁平化 (Flatten): 把二维图像变成一维数组 (1行, 2500列)
        img = img.reshape(1, 1); 
        
        // 3. 转换数据类型为浮点数 (KNN 需要 float)
        img.convertTo(img, CV_32F);

        // 4. 添加到训练集
        trainData.push_back(img);
        trainLabels.push_back(label);
    }
}

int main() {
    Mat trainData;
    Mat trainLabels;

    cout << "开始读取数据..." << endl;

    // 0 代表 A, 1 代表 B, 2 代表 C
    // 请确保你的文件夹路径正确
    load_images("dataset/A", 0, trainData, trainLabels);
    load_images("dataset/B", 1, trainData, trainLabels);
    load_images("dataset/C", 2, trainData, trainLabels);

    if (trainData.empty()) {
        cerr << "错误: 没有加载到任何数据！请检查 dataset 文件夹。" << endl;
        return -1;
    }

    cout << "数据加载完毕。正在训练 KNN 模型..." << endl;

    // 创建 KNN 模型
    Ptr<KNearest> knn = KNearest::create();
    
    // 设置 K 值（看最近的 5 个邻居）
    knn->setDefaultK(5);
    
    // 这是一个分类问题，不是回归问题
    knn->setIsClassifier(true);

    // 开始训练 (其实 KNN 的训练就是把数据存起来)
    knn->train(trainData, ROW_SAMPLE, trainLabels);

    cout << "训练完成！" << endl;

    // 保存模型到文件
    knn->save("knn_model.xml");
    cout << "模型已保存为 'knn_model.xml'" << endl;

    return 0;
}
