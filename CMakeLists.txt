# ?????·Ú???????? Warning
cmake_minimum_required(VERSION 3.10)
project(SignLanguageTranslator)

# SignDatabase.hpp uses C++17 filesystem.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ??? OpenCV ???????
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

include(CTest)
enable_testing()

# ==========================================
# 1. ??????? (????????????)
# ==========================================
add_executable(CaptureImages capture_images.cpp)
target_link_libraries(CaptureImages ${OpenCV_LIBS} Threads::Threads)

# ==========================================
# 2. ?????????? (???????????)
# ??????????? CameraManager.cpp ???????????
# ==========================================
add_executable(MainApp main.cpp CameraManager.cpp GestureRecognizer.cpp VoiceSynthesizer.cpp)
target_link_libraries(MainApp ${OpenCV_LIBS} Threads::Threads)

# ==========================================
# 3. ??????? (???????? knn_model.xml)
# ==========================================
add_executable(TrainApp train.cpp)
target_link_libraries(TrainApp ${OpenCV_LIBS})

# ==========================================
# 4. ??????? (???????? Reliability / Testing)
#    ??????????????? GestureRecognizer£¨KNN + knn_model.xml£©???????????????
# ==========================================
add_executable(GestureRecognizerUnitTests GestureRecognizerUnitTests.cpp GestureRecognizer.cpp)
target_link_libraries(GestureRecognizerUnitTests ${OpenCV_LIBS})
add_test(NAME GestureRecognizerUnitTests COMMAND GestureRecognizerUnitTests)
