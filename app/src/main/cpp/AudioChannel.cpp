//
// Created by zuo on 2022/6/15/015.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int streamIndex, AVCodecContext *codecContext) :
BaseChannel(streamIndex,codecContext) {
    //通道数 2个
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    //采样位 2字节
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    //采样率
    out_sample_rate = 44100;

    //计算转化或数据的最大字节数  44100 * 2 * 2 =176,400
    out_buffers_size = out_sample_rate * out_sample_size * out_channels;
    out_buffer = (uint8_t *) malloc(out_buffers_size);

    // FFmpeg 音频重采样，
    swrContext = swr_alloc_set_opts(0,
                                 //下面是输出环节
                                 AV_CH_LAYOUT_STEREO,//
                                 AV_SAMPLE_FMT_S16,
                                 out_sample_rate,

                                 //下面是输入环节
                                 codecContext->channel_layout,
                                 codecContext->sample_fmt,
                                 codecContext->sample_rate,
                                 0,0);
    //初始化重采样上下午
    swr_init(swrContext);
}

AudioChannel::~AudioChannel() {

}



void *audioDecode_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return nullptr;
}

void *audioPlay_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->_play();
    return nullptr;
}

void AudioChannel::play() {
    LOGE("AudioChannel::play()");
    isPlaying = true;
    // 开启队列的工作
    setEnable(true);
    //解码
    pthread_create(&audioDecodeTask, nullptr, audioDecode_t, this);
    //播放
    pthread_create(&audioPlayTask, nullptr, audioPlay_t, this);
}

void AudioChannel::decode(){
    LOGE("AudioChannel::decode");
    AVPacket *packet = nullptr;
    while (isPlaying) {
        //取出一个数据包,阻塞
        // 1、能够取到数据
        // 2、停止播放
        int ret = packetsQueue.deQueue(packet);
        if (!isPlaying) {
            break;// 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }

        //取出失败
        if (!ret) {
            continue;
        }

        // 向解码器发送解码数据
        ret = avcodec_send_packet(avCodecContext, packet);
        // FFmpeg源码内部 缓存了一份packet副本，所以我才敢大胆的释放
        releaseAVPacket(&packet);

        if (ret == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (ret != 0) {
            break;// 出错误了
        }

        //从解码器取出解码好的数据
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        //需要更多的数据才能够进行解码
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }

        //再开一个线程 来播放 (流畅度)
        frameQueue.enQueue(frame);// 加入队列--PCM数据

    }
    releaseAVPacket(&packet);
}

void AudioChannel::_play() {
    LOGE("AudioChannel::_play");
    initOpenSL();
}

// 6、启动回调函数
void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bq, void * context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    int pcmSize = audioChannel->getPCMData();
//    LOGE("pcmBufferCallBack, pcmSize=%d", pcmSize);
    if(pcmSize > 0){
        (*bq)->Enqueue(bq,
                       audioChannel->out_buffer,//
                       pcmSize);
    }
}

int AudioChannel::getPCMData() {
    int pcmDataSize = 0;
    AVFrame *frame = 0;
    while (isPlaying){
        int ret = frameQueue.deQueue(frame);
        if(!isPlaying){
            break;
        }
        if(!ret){
            continue;
        }
        //转换
        int samples_per_channel = swr_convert(swrContext, &out_buffer, out_buffers_size,
                             (const uint8_t **) frame->data, frame->nb_samples);
        pcmDataSize = samples_per_channel * out_sample_size * out_channels;
        //播放这一段声音的时刻
        double clock = frame->pts * av_q2d(time_base);
        break;
    }
    releaseAVFrame(&frame);
    return pcmDataSize;
}

int AudioChannel::initOpenSL() {
    // 1、创建引擎与接口
    SLresult ret = createEngine();
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("createEngine() failed.");
        return ret;
    }

    // 2、设置混音器
    //通过引擎接口创建混音器outputMixObject
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    ret = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
//    ret = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0,0, 0);
    if(ret != SL_RESULT_SUCCESS) {
        LOGE("CreateOutputMix failed, ret=%d", ret);
        return ret;
    }

    // 初始化混音器outputMixObject
    ret = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if(ret != SL_RESULT_SUCCESS) {
        LOGE("Realize failed, result=%d", ret);
        return ret;
    }

    // 3、创建播放器
    /**
     * 配置输入声音信息
     */
    //创建buffer缓冲类型的队列作为数据定位器(获取播放数据) 2个缓冲区
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};

    //pcm数据格式: pcm、声道数、采样率、采样位、容器大小、通道掩码(双声道)、字节序(小端)
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM,
                            2,
                            SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};

    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    //开启功能
    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //创建播放器
    ret = (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource, &audioSnk, 1, ids, req);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("CreateAudioPlayer() failed.");
        return ret;
    }
    //初始化播放器
    ret = (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("pcmPlayerObject Realize() failed.");
        return ret;
    }

    // 4、设置播放回调
    //获得播放数据队列操作接口
    //注册回调缓冲区 //获取缓冲队列接口
    ret = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("pcmPlayerObject GetInterface(SL_IID_BUFFERQUEUE) failed.");
        return ret;
    }

    LOGE("==RegisterCallback==%d,bqPlayerBufferQueue=%p",ret,bqPlayerBufferQueue);
    //设置回调(启动播放器后执行回调来获取数据并播放)
    ret = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, pcmBufferCallBack, this);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("SLAndroidSimpleBufferQueueItf RegisterCallback() failed.");
        return ret;
    }

    _start();
    return ret;
}

void AudioChannel::_start() {
    //获取播放状态接口
    SLresult ret = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("pcmPlayerObject GetInterface(SL_IID_PLAY) failed.");
        return;
    }

    //设置播放状态
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    // 主动调用回调函数开始工作
    pcmBufferCallBack(bqPlayerBufferQueue, this);
}

SLresult AudioChannel::createEngine() {
    LOGE("AudioChannel::createEngine()");
    SLresult result;
    // 创建引擎engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("slCreateEngine failed, result=%d", result);
        return result;
    }
    // 初始化引擎engineObject
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("engineObject Realize failed, result=%d", result);
        return result;
    }

    // 获取引擎接口engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,&engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("engineObject GetInterface failed, result=%d", result);
        return result;
    }
    return result;
}

void AudioChannel::stop() {

}

