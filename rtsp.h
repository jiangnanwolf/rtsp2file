#ifndef _RTSP_H_
#define _RTSP_H_

#define __STDC_CONSTANT_MACROS
extern "C" 
{
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}

#include <string>
#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

#include "util.h"

using namespace std;

class Rtsp2File
{
  const string input;
  const string output;

  const AVOutputFormat *ofmt = NULL;
  AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
  AVPacket *pkt = NULL;
  const char *in_filename, *out_filename;
  int stream_index = 0;
  int video_index = -1;
  int *stream_mapping = NULL;
  int stream_mapping_size = 0;
  const AVCodec *dec = NULL;
  AVCodecContext *dec_ctx = NULL;
  AVFrame *frame = NULL;

  AVFrame2Mat avframe2mat;
  cv::Mat prev_frame;

  void init();

  bool decode_packet(AVCodecContext *dec, const AVPacket *pkt);
  bool detectMovement(cv::Mat &curr_frame);

public:
  Rtsp2File(const string& input, const string& fileName);
  ~Rtsp2File();

  void run();
  void startThreadPool();
};

#endif