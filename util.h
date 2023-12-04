#ifndef _UTIL_H_
#define _UTIL_H_


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include <libavutil/imgutils.h>
}
#include <opencv2/core.hpp>


AVFrame *cvmatToAvframe(cv::Mat *image, AVFrame *frame);

class AVFrame2Mat
{
    int m_width;
    int m_height;
    SwsContext *m_swsCtx;
public:
    AVFrame2Mat();
    ~AVFrame2Mat();
    cv::Mat operator()(const AVFrame *frame);
};

#endif