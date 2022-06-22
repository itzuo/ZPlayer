//
// Created by zuo on 2022/6/15/015.
//

#ifndef ZPLAYER_BASECHANNEL_H
#define ZPLAYER_BASECHANNEL_H

#include "safe_queue.h"
#include "macro.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/time.h>
};

class BaseChannel{
public:
    BaseChannel(int streamIndex,AVCodecContext *avCodecContext)
    :streamIndex(streamIndex),avCodecContext(avCodecContext){
        packetsQueue.setReleaseHandle(releaseAVPacket);
        frameQueue.setReleaseHandle(releaseAVFrame);
    }

    ~BaseChannel(){
        if (avCodecContext) {
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
        packetsQueue.clear();
        frameQueue.clear();
    };
    /**
     * 释放 队列中 所有的 AVPacket *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVPacket(AVPacket ** packet) {
        if (packet) {
            av_packet_free(packet); // 释放队列里面的 T == AVPacket
            *packet = 0;
        }
    }

    /**
     * 释放 队列中 所有的 AVFrame *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVFrame(AVFrame ** frame) {
        if (frame) {
            av_frame_free(frame); // 释放队列里面的 T == AVFrame
            *frame = 0;
        }
    }

    void setEnable(bool enable){
        packetsQueue.setEnable(enable);
        frameQueue.setEnable(enable);
    }

    //纯虚方法 相当于 抽象方法
    virtual void play() = 0;

    virtual void stop() = 0;

    virtual void decode() = 0;

public:
    int streamIndex;
    AVCodecContext *avCodecContext = 0;
    //编码数据包队列
    SafeQueue<AVPacket *> packetsQueue;
    //解码数据包队列
    SafeQueue<AVFrame *> frameQueue;
    // 音频和视频都会有的标记 是否播放
    bool isPlaying = false;
};

#endif //ZPLAYER_BASECHANNEL_H
