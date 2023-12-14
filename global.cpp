#include "global.h"

ThreadSafeQueue<cv::Mat> g_queue;

std::atomic_bool g_quit;