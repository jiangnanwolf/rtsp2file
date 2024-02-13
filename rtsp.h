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
  const string input;
  const string output;

public:
  Rtsp2File(const string& input, const string& fileName);
  ~Rtsp2File();

  void run();
  void startThreadPool();
};

#endif