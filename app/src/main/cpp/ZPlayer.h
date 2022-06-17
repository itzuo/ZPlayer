//
// Created by zuo on 2022/6/15/015.
//

#ifndef ZPLAYER_ZPLAYER_H
#define ZPLAYER_ZPLAYER_H

#include <pthread.h>
#include "VideoChannel.h"
#include "AudioChannel.h"
#include "JavaCallHelper.h"
#include "macro.h"

extern  "C" {
#include <libavformat/avformat.h>
}

class ZPlayer {

    //友元函数
    friend void *task_prepare(void *args);
    friend void *start_t(void *args);
    friend void* async_stop(void* args);

public:
    ZPlayer(JavaCallHelper *callHelper);
    ~ZPlayer();
    void setDataSource(const char *dataSource);

    void prepare();

    void setWindow(ANativeWindow *window);

    void start();

    void stop();

    void release();

private:
    void _prepare();
    void _start();

private:
    char *dataSource = 0;
    pthread_t prepareTask;
    pthread_t pid_stop;
    AVFormatContext *avFormatContext = 0;
    JavaCallHelper * callHelper = 0;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    bool isPlaying;
    pthread_t startTask;

    ANativeWindow *window = 0;

    int duration;
};


#endif //ZPLAYER_ZPLAYER_H
