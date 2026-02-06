cmake_minimum_required(VERSION 3.20)
project(opencv_test)

set(CMAKE_CXX_STANDARD 17)

# 这里替换成你自己的OpenCV安装路径
set(OpenCV_DIR "C:/opencv/mingw/install/x64/mingw/lib")
find_package(OpenCV REQUIRED)

add_subdirectory(googletest)
include_directories(googletest/googletest/include googletest/googlemock/include)

# 3. 构建主程序：实现手语眼镜实时功能 [cite: 47, 50]

add_executable(sign_language_app
        main.cpp
        CameraManager.cpp
        GestureRecognizer.cpp

)
target_link_libraries(sign_language_app ${OpenCV_LIBS} pthread)

# 4. 构建测试程序：专门运行你的单元测试
# 将测试逻辑与主逻辑分开，是专业代码结构的体现
add_executable(runTests SignDatabaseTest.cpp)
target_link_libraries(runTests GTest::gtest GTest::gtest_main pthread)