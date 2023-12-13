cmake_minimum_required(VERSION 3.15)

project(remux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ENABLE_EXPORTS true)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")

find_package(OpenCV REQUIRED objdetect)

add_executable(remux main.cpp rtsp.cpp util.cpp crash_handler.cpp)

target_link_libraries(remux
    avformat
    avcodec
    swscale
    avutil
    pthread
    opencv_core
    opencv_highgui
    ${OpenCV_LIBS}
)