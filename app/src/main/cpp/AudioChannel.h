//
// Created by zuo on 2022/6/15/015.
//

#ifndef ZPLAYER_AUDIOCHANNEL_H
#define ZPLAYER_AUDIOCHANNEL_H
#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
extern "C" {
#include <libswresample/swresample.h>
};

class AudioChannel : public BaseChannel{
    friend void *audioPlay_t(void *args);
    friend void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf queue, void *pContext);

public:
    AudioChannel(int streamIndex, AVCodecContext * codecContext,AVRational timeBase);

    virtual ~AudioChannel();

    virtual void play();
    virtual void stop();
    virtual void decode();

private:
    void _play();
    int initOpenSL();
    int getPCMData();
    SLresult createEngine();
    void _OpenSLESStart();
private:
    pthread_t audioDecodeTask;
    pthread_t audioPlayTask;

    /**
     * OpenSL ES
     */
    // 引擎
    SLObjectItf engineObject = 0;
    // 引擎接口
    SLEngineItf engineEngine = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf pcmPlayerObject = 0;
    //播放器接口
    SLPlayItf pcmPlayerPlay = 0;
    //队列结构
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = 0;
//    AVRational time_base;

    uint8_t * out_buffer = NULL;
    int out_channels;
    int out_sample_size;
    int out_sample_rate;
    int out_buffers_size;
    SwrContext *swrContext = 0 ;

public:
    double audioTime;
};


#endif //ZPLAYER_AUDIOCHANNEL_H
