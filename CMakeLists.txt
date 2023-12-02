cmake_minimum_required(VERSION 3.15)

project(remux)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)

add_executable(remux main.cpp rtsp.cpp util.cpp)

target_link_libraries(remux
    avformat
    avcodec
    swscale
    avutil
    pthread
    opencv_core
    opencv_highgui
)