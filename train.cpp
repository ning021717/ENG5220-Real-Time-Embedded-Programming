#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;
using namespace cv::ml;

int main() {
    // 1. 设置数据集路径和识别的字母范围
    string path = "dataset/";
    vector<Mat> trainData;
    vector<int> trainLabels;

    // HOG 特征提取器 (参数必须与预测程序完全一致)
    HOGDescriptor hog(Size(64, 64), Size(16, 16), Size(8, 8), Size(8, 8), 9);

    cout << "📂 开始读取数据集..." << endl;

    // 2. 遍历 A-Z 文件夹
    for (int i = 0; i < 26; i++) {
        char folderName = 'A' + i;
        string folderPath = path + folderName + "/";
        
        vector<String> filenames;
        glob(folderPath + "*.jpg", filenames); // 读取文件夹下所有 jpg 格式图片

        if (filenames.empty()) {
            cout << "⚠️ 警告: 文件夹 " << folderName << " 为空，跳过。" << endl;
            continue;
        }

        cout << "正在处理字母 " << folderName << " (共 " << filenames.size() << " 张图片)" << endl;

        for (const auto& file : filenames) {
            Mat img = imread(file, IMREAD_GRAYSCALE);
            if (img.empty()) continue;

            // 统一缩放到 64x64
            resize(img, img, Size(64, 64));

            // 提取 HOG 特征
            vector<float> descriptors;
            hog.compute(img, descriptors);

            trainData.push_back(Mat(descriptors).t());
            trainLabels.push_back(i); // 标签：A=0, B=1...
        }
    }

    if (trainData.empty()) {
        cout << "❌ 错误：没有提取到任何训练数据！请检查 dataset 文件夹。" << endl;
        return -1;
    }

    // 3. 将数据转换为 OpenCV 机器学习需要的格式
    Mat trainMat;
    vconcat(trainData, trainMat); // 合并所有特征行
    trainMat.convertTo(trainMat, CV_32F);
    Mat labelMat(trainLabels);

    // 4. 创建并训练 KNN 模型
    cout << "🧠 正在训练 KNN 模型，请稍候..." << endl;
    Ptr<KNearest> knn = KNearest::create();
    knn->setDefaultK(3); // 设置 K 值为 3
    knn->setIsClassifier(true);
    knn->train(trainMat, ROW_SAMPLE, labelMat);

    // 5. 保存模型
    knn->save("knn_model.xml");
    cout << "✅ 训练完成！模型已保存为: knn_model.xml" << endl;

    return 0;
}
