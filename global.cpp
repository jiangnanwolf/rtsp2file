#include "global.h"

ThreadSafeQueue<cv::Mat> g_queue(3);

std::atomic_bool g_quit;