//
// Created by zuo on 2022/6/15/015.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int streamIndex, AVCodecContext *codecContext) : BaseChannel(streamIndex,
                                                                                        codecContext) {

}

AudioChannel::~AudioChannel() {

}

void AudioChannel::decode(){

}

void AudioChannel::play() {

}

void AudioChannel::stop() {

}

