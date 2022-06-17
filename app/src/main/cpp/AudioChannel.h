//
// Created by zuo on 2022/6/15/015.
//

#ifndef ZPLAYER_AUDIOCHANNEL_H
#define ZPLAYER_AUDIOCHANNEL_H
#include "BaseChannel.h"

class AudioChannel : public BaseChannel{
public:
    AudioChannel(int streamIndex, AVCodecContext * codecContext);

    virtual ~AudioChannel();

    virtual void play();
    virtual void stop();
    virtual void decode();
};


#endif //ZPLAYER_AUDIOCHANNEL_H
