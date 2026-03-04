#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;
using namespace cv::ml;

int main() {
    // 1. Set the dataset path and the range of letters to be recognized.
    string path = "dataset/";
    vector<Mat> trainData;
    vector<int> trainLabels;

    // HOG detect
    HOGDescriptor hog(Size(64, 64), Size(16, 16), Size(8, 8), Size(8, 8), 9);

    cout << "📂 开始读取数据集..." << endl;

    // 2. Traverse folders A-Z
    for (int i = 0; i < 26; i++) {
        char folderName = 'A' + i;
        string folderPath = path + folderName + "/";
        
        vector<String> filenames;
        glob(folderPath + "*.jpg", filenames); 

        if (filenames.empty()) {
            cout << "⚠️ 警告: 文件夹 " << folderName << " 为空，跳过。" << endl;
            continue;
        }

        cout << "正在处理字母 " << folderName << " (共 " << filenames.size() << " 张图片)" << endl;

        for (const auto& file : filenames) {
            Mat img = imread(file, IMREAD_GRAYSCALE);
            if (img.empty()) continue;

            //resize 64
            resize(img, img, Size(64, 64));

            // put HOG figures
            vector<float> descriptors;
            hog.compute(img, descriptors);

            trainData.push_back(Mat(descriptors).t());
            trainLabels.push_back(i); // labeling：A=0, B=1...
        }
    }

    if (trainData.empty()) {
        cout << "❌ 错误：没有提取到任何训练数据！请检查 dataset 文件夹。" << endl;
        return -1;
    }

    // 3. Convert the data into the format required by OpenCV machine learning.
    Mat trainMat;
    vconcat(trainData, trainMat); // 合并所有特征行
    trainMat.convertTo(trainMat, CV_32F);
    Mat labelMat(trainLabels);

    // 4. Create and train a KNN model
    cout << "🧠 正在训练 KNN 模型，请稍候..." << endl;
    Ptr<KNearest> knn = KNearest::create();
    knn->setDefaultK(3); // 设置 K 值为 3
    knn->setIsClassifier(true);
    knn->train(trainMat, ROW_SAMPLE, labelMat);

    // 5. Save Model
    knn->save("knn_model.xml");
    cout << "✅ 训练完成！模型已保存为: knn_model.xml" << endl;

    return 0;
}
