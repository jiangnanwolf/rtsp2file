#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <opencv2/opencv.hpp>
#include "ThreadSafeQueue.h"

extern ThreadSafeQueue<cv::Mat> g_queue;


#endif