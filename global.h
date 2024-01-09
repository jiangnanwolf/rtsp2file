#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <atomic>
#include <opencv2/opencv.hpp>
#include "q.h"

extern ThreadSafeQueue<cv::Mat> g_queue;
extern std::atomic_bool g_quit;
extern std::atomic_int g_frame_count;


#endif