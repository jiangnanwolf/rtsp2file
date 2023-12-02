cmake_minimum_required(VERSION 3.15)

project(remux)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)

add_executable(remux main.cpp rtsp.cpp util.cpp)

target_link_libraries(remux
    avdevice
    xcb
    asound
    SDL2
    sndio
    Xv
    Xext
    m
    avfilter
    postproc
    avformat
    avcodec
    lzma
    x264
    x265
    z
    swresample
    swscale
    avutil
    pthread
    atomic
    X11
    opencv_core
    opencv_highgui
)