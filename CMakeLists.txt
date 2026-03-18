# 升级版本号以消除 Warning
cmake_minimum_required(VERSION 3.10)
project(SignLanguageTranslator)

# 寻找 OpenCV 和多线程库
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

# ==========================================
# 1. 采集工具 (用于录制数据集)
# ==========================================
add_executable(CaptureImages capture_images.cpp)
target_link_libraries(CaptureImages ${OpenCV_LIBS} Threads::Threads)

# ==========================================
# 2. 核心主程序 (用于实时识别展示)
# 注意：这里必须把 CameraManager.cpp 也加进去一起编译！
# ==========================================
add_executable(MainApp main.cpp CameraManager.cpp GestureRecognizer.cpp VoiceSynthesizer.cpp)
target_link_libraries(MainApp ${OpenCV_LIBS} Threads::Threads)

# ==========================================
# 3. 训练工具 (用于生成 knn_model.xml)
# ==========================================
add_executable(TrainApp train.cpp)
target_link_libraries(TrainApp ${OpenCV_LIBS})
