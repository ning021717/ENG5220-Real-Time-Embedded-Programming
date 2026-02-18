cmake_minimum_required(VERSION 3.0)
project(SignLanguageDetector)

find_package(OpenCV REQUIRED)

add_executable(SignLanguage main.cpp)
target_link_libraries(SignLanguage ${OpenCV_LIBS})

add_executable(TrainModel train.cpp)
target_link_libraries(TrainModel ${OpenCV_LIBS})
