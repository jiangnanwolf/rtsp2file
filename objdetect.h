#ifndef _OBJDETECT_H_
#define _OBJDETECT_H_

#include <opencv2/opencv.hpp>


class ObjDetect
{
private:
    cv::dnn::Net m_net;
    std::vector<cv::String> getOutputsNames(const cv::dnn::Net& net);
public:
    ObjDetect();
    void run(int idx);
};

#endif