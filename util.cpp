#include "util.h"

AVFrame2Mat::AVFrame2Mat() : m_width(0), m_height(0), m_swsCtx(nullptr)
{
}

AVFrame2Mat::~AVFrame2Mat()
{
  sws_freeContext(m_swsCtx);
}

void AVFrame2Mat::init(int width, int height, AVPixelFormat format)
{
  m_width = width;
  m_height = height;
  m_swsCtx = sws_getContext(
      width, height, format, width, height, AVPixelFormat::AV_PIX_FMT_BGR24,
      SWS_FAST_BILINEAR, NULL, NULL, NULL);
}

cv::Mat AVFrame2Mat::convert(const AVFrame *frame)
{
  cv::Mat image(m_height, m_width, CV_8UC3);
  int cvLinesizes[1];
  cvLinesizes[0] = image.step1();
  sws_scale(m_swsCtx, frame->data, frame->linesize, 0, m_height, &image.data,
            cvLinesizes);
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