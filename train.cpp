#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
#include <vector>
#include <string>

using namespace cv;
using namespace cv::ml;
using namespace std;

const int IMG_SIZE = 50; 

// Dynamic loader: Returns true if folder exists and has images
bool load_images(string directory, int label, Mat& trainData, vector<int>& trainLabels) {
    vector<String> filenames;
    
    // [CRITICAL FIX] Catch OpenCV 4.10 exception if the folder does not exist at all
    try {
        glob(directory + "/*.jpg", filenames); 
    } catch (const cv::Exception& e) {
        return false; // Folder is missing, safely return false to skip it
    }

    if(filenames.empty()) {
        return false; // Folder exists but is empty, safely skip
    }

    cout << "[INFO] Processing folder " << directory << " (" << filenames.size() << " images)..." << endl;

    for (size_t i = 0; i < filenames.size(); ++i) {
        // Read strictly as Grayscale since we are loading binary masks
        Mat img = imread(filenames[i], IMREAD_GRAYSCALE); 
        
        if (img.empty()) continue;

        resize(img, img, Size(IMG_SIZE, IMG_SIZE));
        img = img.reshape(1, 1); 
        img.convertTo(img, CV_32F);

        trainData.push_back(img);
        trainLabels.push_back(label);
    }
    return true;
}

int main() {
    Mat trainData;
    vector<int> trainLabels;

    cout << "[INFO] Starting dynamic data loader..." << endl;
    int classesLoaded = 0;

    // Scan through A to Z dynamically
    for (int i = 0; i < 26; ++i) {
        char letter = 'A' + i;
        string folder = "dataset/" + string(1, letter);
        
        if (load_images(folder, i, trainData, trainLabels)) {
            classesLoaded++;
        }
    }

    if (trainData.empty()) {
        cerr << "[ERROR] No data found! Please record some images first." << endl;
        return -1;
    }

    cout << "--------------------------------------" << endl;
    cout << "[INFO] Total classes loaded: " << classesLoaded << endl;
    cout << "[INFO] Total training samples: " << trainData.rows << endl;
    cout << "[INFO] Training KNN model. Please wait..." << endl;

    // Initialize KNN Machine Learning model
    Ptr<KNearest> knn = KNearest::create();
    knn->setDefaultK(5);
    knn->setIsClassifier(true);
    
    // Convert labels vector to strictly formatted OpenCV Matrix
    Mat labelsMat(trainLabels);
    labelsMat.convertTo(labelsMat, CV_32S);

    Ptr<TrainData> trainingData = TrainData::create(trainData, ROW_SAMPLE, labelsMat);
    knn->train(trainingData);

    knn->save("knn_model.xml");
    cout << "[SUCCESS] Model trained and saved as knn_model.xml!" << endl;
    cout << "--------------------------------------" << endl;

    return 0;
}