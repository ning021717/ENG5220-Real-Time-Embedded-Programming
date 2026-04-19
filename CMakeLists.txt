cmake_minimum_required(VERSION 3.10)
project(SignLanguageTranslator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# ==========================================
# Dependencies
# ==========================================
find_package(PkgConfig REQUIRED)
find_package(OpenCV    REQUIRED)
find_package(Threads   REQUIRED)

# libcamera (Raspberry Pi camera stack — required for event-driven capture)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
message(STATUS "libcamera version : ${LIBCAMERA_VERSION}")
message(STATUS "libcamera libs    : ${LIBCAMERA_LINK_LIBRARIES}")

# cam2opencv wrapper (install from libcamera2opencv-1.0:
#   cmake . && make && sudo make install)
find_library(CAM2OPENCV_LIB  NAMES cam2opencv REQUIRED)
find_path   (CAM2OPENCV_INC  NAMES libcam2opencv.h REQUIRED)
message(STATUS "cam2opencv lib    : ${CAM2OPENCV_LIB}")
message(STATUS "cam2opencv include: ${CAM2OPENCV_INC}")

include(CTest)
enable_testing()

# ==========================================
# 1. CaptureImages — data collection tool
# ==========================================
add_executable(CaptureImages capture_images.cpp)
target_include_directories(CaptureImages PRIVATE
    ${CAM2OPENCV_INC}
    ${LIBCAMERA_INCLUDE_DIRS})
target_link_libraries(CaptureImages
    ${CAM2OPENCV_LIB}
    PkgConfig::LIBCAMERA
    ${OpenCV_LIBS}
    Threads::Threads)

# ==========================================
# 2. MainApp — real-time sign-language pipeline
# ==========================================
add_executable(MainApp
    main.cpp
    CameraManager.cpp
    GestureRecognizer.cpp
    VoiceSynthesizer.cpp)
target_include_directories(MainApp PRIVATE
    ${CAM2OPENCV_INC}
    ${LIBCAMERA_INCLUDE_DIRS})
target_link_libraries(MainApp
    ${CAM2OPENCV_LIB}
    PkgConfig::LIBCAMERA
    ${OpenCV_LIBS}
    Threads::Threads)

# ==========================================
# 3. TrainApp — offline KNN model training
# ==========================================
add_executable(TrainApp train.cpp)
target_link_libraries(TrainApp ${OpenCV_LIBS})

# ==========================================
# 4. GestureRecognizerUnitTests — CI-compatible unit tests
# ==========================================
add_executable(GestureRecognizerUnitTests
    GestureRecognizerUnitTests.cpp
    GestureRecognizer.cpp)
target_link_libraries(GestureRecognizerUnitTests ${OpenCV_LIBS})
add_test(NAME GestureRecognizerUnitTests COMMAND GestureRecognizerUnitTests)
