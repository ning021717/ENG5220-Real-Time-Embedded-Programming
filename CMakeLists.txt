cmake_minimum_required(VERSION 3.10)
project(SignLanguageTranslator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# ==========================================
# Dependencies
# ==========================================
find_package(PkgConfig)
find_package(OpenCV  REQUIRED)
find_package(Threads REQUIRED)

# libcamera and cam2opencv are Raspberry Pi-only ù optional so that
# GestureRecognizerUnitTests can still build on CI (Ubuntu, no libcamera).
set(HAVE_LIBCAMERA OFF)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBCAMERA IMPORTED_TARGET libcamera)
    if(LIBCAMERA_FOUND)
        find_library(CAM2OPENCV_LIB NAMES cam2opencv)
        find_path   (CAM2OPENCV_INC NAMES libcam2opencv.h)
        find_library(TURBOJPEG_LIB  NAMES turbojpeg)
        if(CAM2OPENCV_LIB AND CAM2OPENCV_INC)
            set(HAVE_LIBCAMERA ON)
            message(STATUS "libcamera ${LIBCAMERA_VERSION} + cam2opencv found ù building Pi targets")
        endif()
    endif()
endif()

if(NOT HAVE_LIBCAMERA)
    message(STATUS "libcamera/cam2opencv not found ù CaptureImages and MainApp skipped (Raspberry Pi only)")
endif()

include(CTest)
enable_testing()

# ==========================================
# 1. CaptureImages ù data collection tool  (Pi only)
# ==========================================
if(HAVE_LIBCAMERA)
    add_executable(CaptureImages capture_images.cpp)
    target_include_directories(CaptureImages PRIVATE
        ${CAM2OPENCV_INC}
        ${LIBCAMERA_INCLUDE_DIRS})
    target_link_libraries(CaptureImages
        ${CAM2OPENCV_LIB}
        ${TURBOJPEG_LIB}
        PkgConfig::LIBCAMERA
        ${OpenCV_LIBS}
        Threads::Threads)
endif()

# ==========================================
# 2. MainApp ù real-time sign-language pipeline  (Pi only)
# ==========================================
if(HAVE_LIBCAMERA)
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
        ${TURBOJPEG_LIB}
        PkgConfig::LIBCAMERA
        ${OpenCV_LIBS}
        Threads::Threads)
endif()

# ==========================================
# 3. TrainApp ù offline KNN model training
# ==========================================
add_executable(TrainApp train.cpp)
target_link_libraries(TrainApp ${OpenCV_LIBS})

# ==========================================
# 4. GestureRecognizerUnitTests ù CI-compatible unit tests
# ==========================================
add_executable(AugmentDataset augment_dataset.cpp)
target_link_libraries(AugmentDataset ${OpenCV_LIBS})

add_executable(GestureRecognizerUnitTests
    GestureRecognizerUnitTests.cpp
    GestureRecognizer.cpp)
target_link_libraries(GestureRecognizerUnitTests ${OpenCV_LIBS})
add_test(NAME GestureRecognizerUnitTests COMMAND GestureRecognizerUnitTests)
