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
using namespace std;

class Rtsp2File
{
  const string rtspUrl;
  const string fileName;

  const AVOutputFormat *ofmt = nullptr;
  AVFormatContext *ifmt_ctx = nullptr, *ofmt_ctx = nullptr;
  AVPacket *pkt = nullptr;
  int ret = 0;
  int stream_index = 0;
  int *stream_mapping = nullptr;
  int m_nb_streams = 0;
  int m_video_stream_index = -1;
  int m_audio_stream_index = -1;
  vector<AVStream*> m_out_streams;
  vector<AVStream*> m_in_streams;
  vector<AVCodec*> m_decoders;
  vector<AVCodecContext*> m_dec_ctxs;

  void init();
  void deinit();

public:
    Rtsp2File(const string& rtspUrl, const string& fileName);
    ~Rtsp2File();

    void run();
};

#endif