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

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           ts2str(pkt->pts).c_str(), ts2timestr(pkt->pts, time_base).c_str(),
           ts2str(pkt->dts).c_str(), ts2timestr(pkt->dts, time_base).c_str(),
           ts2str(pkt->duration).c_str(), ts2timestr(pkt->duration, time_base).c_str(),
           pkt->stream_index);
}

static string err2str(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return errbuf;
}

Codec::Codec() : m_out_streams(nullptr), m_in_streams(nullptr), m_dec_ctxs(nullptr)
{
}

Codec::~Codec()
{
    if(m_dec_ctxs){
        avcodec_free_context(&m_dec_ctxs);
    }
}

Rtsp2File::Rtsp2File(const string& rtspUrl, const string& fileName):
    rtspUrl(rtspUrl),
    fileName(fileName)
{
    init();
}


Rtsp2File::~Rtsp2File()
{
    deinit();
}

void Rtsp2File::init()
{
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate AVPacket\n");
        return;
    }

    if ((ret = avformat_open_input(&ifmt_ctx, rtspUrl.c_str(), 0, 0)) < 0) {
        cerr << "Could not open input file " << rtspUrl << endl;
        return;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        cerr << "Could not open input file " << rtspUrl << endl;
        return;
    }

    av_dump_format(ifmt_ctx, 0, rtspUrl.c_str(), 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, fileName.c_str());
    if (!ofmt_ctx) {
        cerr << "Could not create output context\n" << endl;
        ret = AVERROR_UNKNOWN;
        return;
    }

    m_nb_streams = ifmt_ctx->nb_streams;
    m_codecs.resize(m_nb_streams);

    ofmt = ofmt_ctx->oformat;
        // Find video stream
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        m_codecs[i].m_in_streams = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = m_codecs[i].m_in_streams->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            continue;
        }
        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_video_stream_index = i;
        }
        if (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audio_stream_index = i;
        }

        m_codecs[i].m_out_streams = avformat_new_stream(ofmt_ctx, NULL);
        if (!m_codecs[i].m_out_streams) {
            cerr << "Failed allocating output stream" << endl;
            ret = AVERROR_UNKNOWN;
            return;
        }

        ret = avcodec_parameters_copy(m_codecs[i].m_out_streams->codecpar, in_codecpar);
        if (ret < 0) {
            cerr << "Failed to copy codec parameters" << endl;
            return;
        }
        m_codecs[i].m_out_streams->codecpar->codec_tag = 0;


        // Find the decoder for the stream
        const AVCodec* decoder = avcodec_find_decoder(m_codecs[i].m_in_streams->codecpar->codec_id);
        if (!decoder) {
            fprintf(stderr, "No decoder found for codec_id\n");
            // Handle error
        }
                cout << "codec: " << decoder << endl;

        // Allocate a new decoding context
        m_codecs[i].m_dec_ctxs = avcodec_alloc_context3(decoder);
        if (!m_codecs[i].m_dec_ctxs) {
            fprintf(stderr, "Could not allocate decoding context\n");
            // Handle error
        }

        // Copy the codec parameters from the input stream to the new decoding context
        if (avcodec_parameters_to_context(m_codecs[i].m_dec_ctxs, m_codecs[i].m_in_streams->codecpar) < 0) {
            fprintf(stderr, "Could not copy parameters to context\n");
            // Handle error
        }

        // Open the codec
        if (avcodec_open2(m_codecs[i].m_dec_ctxs, decoder, NULL) < 0) {
            fprintf(stderr, "Could not open codec\n");
            // Handle error
        }
    }


    av_dump_format(ofmt_ctx, 0, fileName.c_str(), 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, fileName.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            cerr << "Could not open output file " << fileName << endl;
            cerr << err2str(ret) << endl;
            return;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        cerr << err2str(ret) << endl;
        return;
    }
}

void Rtsp2File::deinit()
{
    av_packet_free(&pkt);

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);


    if (ret < 0 && ret != AVERROR_EOF) {
        cerr << "Error occurred: " << err2str(ret);
    }
}



void Rtsp2File::startThreadPool()
{
    unsigned int ncores = std::thread::hardware_concurrency();

    // thread t1([]() {
    //     ObjDetect detect;
    //     detect.run();
    // });
    // t1.detach();


    thread t2([]() {
        MovDetect detect;
        detect.run();
    });
    t2.detach();

}

void Rtsp2File::run()
{
    // Define a comparison function for the priority queue
    auto compare = [](AVPacket* a, AVPacket* b) {
        return a->dts > b->dts;
    };
    vector<priority_queue<AVPacket*, vector<AVPacket*>, decltype(compare)>> buffer(m_nb_streams, priority_queue<AVPacket*, std::vector<AVPacket*>, decltype(compare)>(compare));

    // Define the window size
    size_t window_size = 3;  // Adjust this value as needed
    AVFrame *frame = NULL;
    vector<int64_t> last_dts(m_nb_streams,INT64_MIN);
    AVFrame2Mat avframe2mat;


    // startThreadPool();
            int64_t frame_number = 0; // The current frame number

    while (1) {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0)
            break;
        // log_packet(ifmt_ctx, pkt, "in");
        in_stream  = ifmt_ctx->streams[pkt->stream_index];
        if (pkt->stream_index >= m_nb_streams ) {
            av_packet_unref(pkt);
            continue;
        }
        if (pkt->dts < last_dts[pkt->stream_index]) {
            av_packet_unref(pkt);
            continue;
        }
        // cout << "stream_index:" << pkt->stream_index << " dts:" << pkt->dts << " last_dts:" << last_dts[pkt->stream_index] << endl;
        last_dts[pkt->stream_index] = pkt->dts;


        out_stream = ofmt_ctx->streams[pkt->stream_index];
        // log_packet(ifmt_ctx, pkt, "in");

        /* copy packet */
        // cout << "in_stream:" << in_stream->time_base.den <<" : "<< in_stream->time_base.num << endl;
        // cout << "out_stream:" << out_stream->time_base.den << " : "<<  out_stream->time_base.num << endl;
        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;
        // // log_packet(ofmt_ctx, pkt, "out");
        
        // if(pkt->stream_index == m_video_stream_index){
        //     // Send the packet to the decoder
        //     if (avcodec_send_packet(m_codecs[pkt->stream_index].m_dec_ctxs, pkt) < 0) {
        //         fprintf(stderr, "Error sending packet to decoder\n");
        //         // Handle error
        //     }

        //     // Receive the decoded frame from the decoder
        //     AVFrame* frame = av_frame_alloc();
        //     if (avcodec_receive_frame(m_codecs[pkt->stream_index].m_dec_ctxs, frame) < 0) {
        //         fprintf(stderr, "Error receiving frame from decoder\n");
        //         // Handle error
        //     }
        //     if(frame->height > 0 && frame->width > 0){
        //         cv::Mat img = avframe2mat(frame);
        //         g_queue.Push(img);

        //     }else{
        //         cout << "frame->height:" << frame->height << " frame->width:" << frame->width << endl;
        //     }
        //     av_frame_free(&frame);
        // }

        // TODO: check if we need to save the frame
        // if (g_frame_count > 0) {
        //     g_frame_count--;
        
        // Assuming you have these values available


            // Calculate the PTS and DTS
            int frame_rate = av_q2d(in_stream->avg_frame_rate);
            pkt->pts = av_rescale_q(frame_number, (AVRational){1, frame_rate}, out_stream->time_base);
            pkt->dts = pkt->pts; // In many cases, DTS and PTS will be the same

            frame_number++;

            ret = av_interleaved_write_frame(ofmt_ctx, pkt);
            /* pkt is now blank (av_interleaved_write_frame() takes ownership of
            * its contents and resets pkt), so that no unreferencing is necessary.
            * This would be different if one used av_write_frame(). */
            if (ret < 0) {
                fprintf(stderr, "Error muxing packet\n");
                break;
            }
        // }

        av_packet_unref(pkt);
    }

    av_write_trailer(ofmt_ctx);

    g_queue.Quit();
}