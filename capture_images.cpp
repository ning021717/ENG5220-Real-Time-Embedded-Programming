#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <sys/stat.h>

using namespace cv;
using namespace std;

int main() {
    // Label for the current dataset category (e.g., gesture/character "Z")
    string label = "Z"; 
    // Directory path to store collected images (organized by label)
    string folder = "dataset/" + label;
    // Create directory (with -p to avoid error if directory already exists)
    system(("mkdir -p " + folder).c_str());

    // Open camera using basic device index (CAP_V4L2 for Linux video4linux2 driver)
    VideoCapture cap(0, CAP_V4L2); 
    
    // Check if camera initialization succeeded
    if (!cap.isOpened()) {
        cerr << "Error: Failed to open camera. Ensure the camera cable is connected properly." << endl;
        return -1;
    }

    // Note: Do NOT set camera parameters (cap.set) here to prevent driver conflict crashes

    // Counter for saved image files
    int count = 0;
    cout << "--- Viewfinder Ready ---" << endl;
    cout << "Operations: Press 's' to capture and save image, press 'q' to exit" << endl;

    Mat frame; // Variable to store each camera frame
    while (true) {
        // Read frame from camera
        cap >> frame;
        // Skip empty frames (avoid processing invalid data)
        if (frame.empty()) continue;

        // Display live viewfinder window
        imshow("Collector (Press 's' to Save)", frame);

        // Handle keyboard input (wait 1ms for key press)
        char key = (char)waitKey(1);
        if (key == 's') {
            // Generate unique filename (directory + counter + .jpg)
            string filename = folder + "/" + to_string(count) + ".jpg";
            // Save current frame to file
            imwrite(filename, frame);
            cout << "Saved: " << filename << endl;
            count++; // Increment counter for next image
        } else if (key == 'q') {
            // Exit loop when 'q' is pressed
            break;
        }
    }

    // Release camera resource and close all OpenCV windows
    cap.release();
    destroyAllWindows();
    return 0;
}
