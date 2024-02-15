#include <queue>
#include <iostream>
#include <thread>

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

#include "rtsp.h"
#include "util.h"
#include "global.h"
#include "objdetect.h"
#include "movdetect.h"

using namespace std;

static string ts2str(int64_t ts)
{
  char buf[AV_TS_MAX_STRING_SIZE];
  av_ts_make_string(buf, ts);
  return buf;
}

static string ts2timestr(int64_t ts, AVRational *tb)
{
  char buf[AV_TS_MAX_STRING_SIZE];
  av_ts_make_time_string(buf, ts, tb);
  return buf;
}

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
  AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

  // printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
  //        tag,
  //        ts2str(pkt->pts).c_str(), ts2timestr(pkt->pts, time_base).c_str(),
  //        ts2str(pkt->dts).c_str(), ts2timestr(pkt->dts, time_base).c_str(),
  //        ts2str(pkt->duration).c_str(), ts2timestr(pkt->duration, time_base).c_str(),
  //        pkt->stream_index);
}

static string err2str(int errnum)
{
  char errbuf[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
  return errbuf;
}

Rtsp2File::Rtsp2File(const string &input, const string &output) : input(input),
                                                                  output(output)
{
}

Rtsp2File::~Rtsp2File()
{
}

void Rtsp2File::startThreadPool()
{
  unsigned int ncores = std::thread::hardware_concurrency();

  // thread t1([]() {
  //     ObjDetect detect;
  //     detect.run();
  // });
  // t1.detach();

  thread t2([]()
            {
      MovDetect detect;
      detect.run(); });
  t2.detach();
}

cv::Mat prev_frame;

bool detectMovement(cv::Mat &curr_frame)
{
  if (prev_frame.empty())
  {
    curr_frame.copyTo(prev_frame);
    return false;
  }

  cv::Mat diff, gray_diff, thresh;
  cv::GaussianBlur(curr_frame, curr_frame, cv::Size(21, 21), 0);
  cv::absdiff(prev_frame, curr_frame, diff);
  cv::cvtColor(diff, gray_diff, cv::COLOR_BGR2GRAY);
  cv::threshold(gray_diff, thresh, 25, 255, cv::THRESH_BINARY);
  cv::erode(thresh, thresh, cv::Mat(), cv::Point(-1, -1), 2);
  cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1, -1), 2);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

  bool movement_detected = false;
  for (const auto &contour : contours)
  {
    if (cv::contourArea(contour) > 200)
    { // adjust this value based on your requirement
      movement_detected = true;
      break;
    }
  }
  cout << "Movement detected: " << movement_detected << endl;
  prev_frame = curr_frame;
  return movement_detected;
}

bool decode_packet(AVCodecContext *dec, const AVPacket *pkt)
{
  int ret = 0;
  AVFrame *frame = av_frame_alloc();

  // submit the packet to the decoder
  ret = avcodec_send_packet(dec, pkt);
  if (ret < 0)
  {
    fprintf(stderr, "Error submitting a packet for decoding (%s)\n", err2str(ret).c_str());
    return false;
  }
  bool movement_detected = false;
  // get all the available frames from the decoder
  while (ret >= 0)
  {
    ret = avcodec_receive_frame(dec, frame);
    if (ret < 0)
    {
      // those two return values are special and mean there is no output
      // frame available, but there were no errors during decoding
      if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
        return movement_detected;

      fprintf(stderr, "Error during decoding (%s)\n", err2str(ret).c_str());
      return false;
    }

    // write the frame data to output file
    if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
    {
      AVFrame2Mat avframe2mat;
      cv::Mat image = avframe2mat(frame);
      // Resize the image
      cv::Mat resizedImg;

      cv::resize(image, resizedImg, cv::Size(320, 200));
      cv::imshow("mov", resizedImg);
      cv::waitKey(1);
      movement_detected = detectMovement(resizedImg);
      if(movement_detected)
      {
        cout <<"Movement detected" << endl;
      } 
    }
    else
      ret = 1; // output_audio_frame(frame);

    av_frame_unref(frame);
    if (ret < 0)
      return false;
  }

  av_frame_free(&frame);

  return movement_detected;
}

void Rtsp2File::run()
{
  const AVOutputFormat *ofmt = NULL;
  AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
  AVPacket *pkt = NULL;
  const char *in_filename, *out_filename;
  int ret, i;
  int stream_index = 0;
  int video_index = -1;
  int *stream_mapping = NULL;
  int stream_mapping_size = 0;

  in_filename = input.c_str();
  out_filename = output.c_str();

  pkt = av_packet_alloc();
  if (!pkt)
  {
    fprintf(stderr, "Could not allocate AVPacket\n");
    return;
  }

  if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0)
  {
    fprintf(stderr, "Could not open input file '%s'", in_filename);
    return;
  }

  if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
  {
    fprintf(stderr, "Failed to retrieve input stream information");
    return;
  }

  av_dump_format(ifmt_ctx, 0, in_filename, 0);

  avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
  if (!ofmt_ctx)
  {
    fprintf(stderr, "Could not create output context\n");
    ret = AVERROR_UNKNOWN;
    return;
  }

  stream_mapping_size = ifmt_ctx->nb_streams;
  stream_mapping = (int *)av_calloc(stream_mapping_size, sizeof(*stream_mapping));
  if (!stream_mapping)
  {
    ret = AVERROR(ENOMEM);
    return;
  }

  ofmt = ofmt_ctx->oformat;

  const AVCodec *dec = NULL;
  AVCodecContext *dec_ctx = NULL;

  for (i = 0; i < ifmt_ctx->nb_streams; i++)
  {
    AVStream *out_stream;
    AVStream *in_stream = ifmt_ctx->streams[i];
    AVCodecParameters *in_codecpar = in_stream->codecpar;

    if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
    {
      stream_mapping[i] = -1;
      continue;
    }

    if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      video_index = i;
      /* find decoder for the stream */
      dec = avcodec_find_decoder(in_codecpar->codec_id);
      if (!dec)
      {
        fprintf(stderr, "Failed to find codec\n");
        return;
      }
      /* Allocate a codec context for the decoder */
      dec_ctx = avcodec_alloc_context3(dec);
      if (!dec_ctx)
      {
        fprintf(stderr, "Failed to allocate the codec context\n");
        return;
      }

      /* Copy codec parameters from input stream to output codec context */
      if ((ret = avcodec_parameters_to_context(dec_ctx, in_codecpar)) < 0)
      {
        fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
        return;
      }

      /* Init the decoders */
      if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0)
      {
        fprintf(stderr, "Failed to open codec\n");
        return;
      }
    }

    stream_mapping[i] = stream_index++;

    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream)
    {
      fprintf(stderr, "Failed allocating output stream\n");
      ret = AVERROR_UNKNOWN;
      return;
    }

    ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
    if (ret < 0)
    {
      fprintf(stderr, "Failed to copy codec parameters\n");
      return;
    }
    out_stream->codecpar->codec_tag = 0;
  }
  av_dump_format(ofmt_ctx, 0, out_filename, 1);

  if (!(ofmt->flags & AVFMT_NOFILE))
  {
    ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
      fprintf(stderr, "Could not open output file '%s'", out_filename);
      return;
    }
  }

  ret = avformat_write_header(ofmt_ctx, NULL);
  if (ret < 0)
  {
    fprintf(stderr, "Error occurred when opening output file\n");
    return;
  }
  int idx = 0;
  int64_t pts_start_0 = 0, pts_start_1 = 0;
  int count = 0;
  while (1)
  {
    AVStream *in_stream, *out_stream;

    ret = av_read_frame(ifmt_ctx, pkt);
    if (ret < 0)
      break;

    in_stream = ifmt_ctx->streams[pkt->stream_index];
    if (pkt->stream_index >= stream_mapping_size ||
        stream_mapping[pkt->stream_index] < 0)
    {
      av_packet_unref(pkt);
      continue;
    }
    bool save = false;
    if (pkt->stream_index == video_index)
      save = decode_packet(dec_ctx, pkt);
    if (save ){
      count = 150;
    }
    if (count < 0) {
      av_packet_unref(pkt);
      continue;
    }

    count--;
    // idx++;
    // if (idx < 1000)
    // {
    //   av_packet_unref(pkt);
    //   continue;
    // }

    pkt->stream_index = stream_mapping[pkt->stream_index];
    out_stream = ofmt_ctx->streams[pkt->stream_index];
    log_packet(ifmt_ctx, pkt, "in");

    /* copy packet */
    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
    if (pkt->stream_index == 0)
    {
      pkt->pts = pts_start_0;
      pkt->dts = pts_start_0;
      pts_start_0 += pkt->duration;
    }
    else if (pkt->stream_index == 1)
    {
      pkt->pts = pts_start_1;
      pkt->dts = pts_start_1;
      pts_start_1 += pkt->duration;
    }
    pkt->pos = -1;
    log_packet(ofmt_ctx, pkt, "out");

    ret = av_interleaved_write_frame(ofmt_ctx, pkt);
    /* pkt is now blank (av_interleaved_write_frame() takes ownership of
     * its contents and resets pkt), so that no unreferencing is necessary.
     * This would be different if one used av_write_frame(). */
    if (ret < 0)
    {
      fprintf(stderr, "Error muxing packet\n");
      break;
    }
  }
  av_write_trailer(ofmt_ctx);
end:
  av_packet_free(&pkt);

  avformat_close_input(&ifmt_ctx);

  /* close output */
  if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
    avio_closep(&ofmt_ctx->pb);
  avformat_free_context(ofmt_ctx);

  av_freep(&stream_mapping);

  if (ret < 0 && ret != AVERROR_EOF)
  {
    fprintf(stderr, "Error occurred: %s\n", err2str(ret).c_str());
    return;
  }

  return;
}