#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <algorithm>
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

        // Bounding-box normalisation: mirror the same transform applied at
        // inference time in GestureRecognizer::predict(). Without this step
        // the 50×50 feature vector encodes pixel position within the full ROI,
        // so KNN classifies by where the hand appears on screen rather than
        // by hand shape. Cropping to the largest blob's bounding box first
        // makes training features position- and scale-invariant.
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (!contours.empty()) {
            auto maxIt = std::max_element(contours.begin(), contours.end(),
                [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                    return cv::contourArea(a) < cv::contourArea(b);
                });
            cv::Rect bbox = cv::boundingRect(*maxIt);
            const int pad = 4;
            bbox.x      = std::max(0, bbox.x - pad);
            bbox.y      = std::max(0, bbox.y - pad);
            bbox.width  = std::min(img.cols - bbox.x, bbox.width  + 2 * pad);
            bbox.height = std::min(img.rows - bbox.y, bbox.height + 2 * pad);
            img = img(bbox).clone();
        }

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