//
// Created by zuo on 2022/6/15/015.
//

#ifndef ZPLAYER_VIDEOCHANNEL_H
#define ZPLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include <android/native_window_jni.h>

extern "C"{
#include <libswscale/swscale.h> // 视频画面像素格式转换的模块
#include <libavutil/imgutils.h>
};

class VideoChannel : public BaseChannel{
    friend void *videoPlay_t(void *args);

public:
    VideoChannel(int streamIndex, AVCodecContext * codecContext);

    virtual ~VideoChannel();

    void setWindow(ANativeWindow *window);

    virtual void play();
    virtual void stop();
    virtual void decode();

private:
    void _play();
    void _onDraw(uint8_t *dst_data, int dst_linesize, int width, int height);

private:
    pthread_t videoDecodeTask, videoPlayTask;
    pthread_mutex_t surfaceMutex;
    ANativeWindow *window = 0;
};


#endif //ZPLAYER_VIDEOCHANNEL_H
