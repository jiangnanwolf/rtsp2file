#ifndef _MOVDETECT_H_
#define _MOVDETECT_H_

#include <opencv2/opencv.hpp>

class MovDetect {
private:
    cv::Mat prev_frame;
    bool detectMovement(cv::Mat& curr_frame);
public:
    MovDetect() {}

    void run();
};

#endif