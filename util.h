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


cv::Mat avframeToCvmat(const AVFrame *frame);
AVFrame *cvmatToAvframe(cv::Mat *image, AVFrame *frame);

#endif