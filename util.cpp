#include "util.h"




cv::Mat avframeToCvmat(const AVFrame *frame) {
  int width = frame->width;
  int height = frame->height;
  cv::Mat image(height, width, CV_8UC3);
  int cvLinesizes[1];
  cvLinesizes[0] = image.step1();
  SwsContext *conversion = sws_getContext(
      width, height, (AVPixelFormat)frame->format, width, height,
      AVPixelFormat::AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  sws_scale(conversion, frame->data, frame->linesize, 0, height, &image.data,
            cvLinesizes);
  sws_freeContext(conversion);
  return image;
}


AVFrame *cvmatToAvframe(cv::Mat *image, AVFrame *frame) {
  int width = image->cols;
  int height = image->rows;
  int cvLinesizes[1];
  cvLinesizes[0] = image->step1();
  if (frame == NULL) {
    frame = av_frame_alloc();
    av_image_alloc(frame->data, frame->linesize, width, height,
                   AVPixelFormat::AV_PIX_FMT_YUV420P, 1);
  }
  SwsContext *conversion = sws_getContext(
      width, height, AVPixelFormat::AV_PIX_FMT_BGR24, width, height,
      (AVPixelFormat)frame->format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  sws_scale(conversion, &image->data, cvLinesizes, 0, height, frame->data,
            frame->linesize);
  sws_freeContext(conversion);
  return frame;
}