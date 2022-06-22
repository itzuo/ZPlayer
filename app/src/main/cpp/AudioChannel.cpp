//
// Created by zuo on 2022/6/15/015.
//

#include "AudioChannel.h"
/**
 * 音频三要素
 * 1.采样率  44100 48000
 * 2.位声/采样格式大小 16bit == 2字节
 * 3.声道数 2 --- 人类就是两个耳朵
 */
AudioChannel::AudioChannel(int streamIndex, AVCodecContext *codecContext) :
BaseChannel(streamIndex,codecContext) {
    /*
     * 音频压缩包 AAC
     *
     * 44100
     * 32bit   算法效率高 浮点运算高
     * 2
    */

    // 重采样 音频格式转换

    /*
    * 44100
    * 16bit   算法效率高 浮点运算高
    * 2
    */

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
                                 AV_CH_LAYOUT_STEREO,//声道布局类型 双声道
                                 AV_SAMPLE_FMT_S16, // 采样大小 16bit
                                 out_sample_rate, // 采样率  44100

                                 //下面是输入环节
                                 codecContext->channel_layout, // 声道布局类型
                                 codecContext->sample_fmt, // 采样大小 32bit  aac
                                 codecContext->sample_rate, // 采样率
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
    //解码, 第一个线程： 音频：取出队列的压缩包进行解码,解码后的原始包 再push队列中去（音频：PCM数据）
    pthread_create(&audioDecodeTask, nullptr, audioDecode_t, this);
    //播放,  第二线线程：音频：从队列取出原始包，播放
    pthread_create(&audioPlayTask, nullptr, audioPlay_t, this);
}

void AudioChannel::decode(){
    LOGE("AudioChannel::decode");
    AVPacket *packet = nullptr;
    while (isPlaying) {
        if(isPlaying && frameQueue.size() > 100){
            av_usleep(10 * 1000);
            continue;
        }
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
        } else if (ret != 0) {
            releaseAVFrame(&frame);
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
                       audioChannel->out_buffer, // PCM数据
                       pcmSize);
    }
}

int AudioChannel::getPCMData() {
    int pcmDataSize = 0;
    AVFrame *frame = 0;

    //  PCM数据在哪里？是在frames队列中  frame->data == PCM数据(待重采样即未重采样的  32bit采用格式 44100采样率 2声道)
    while (isPlaying){
        int ret = frameQueue.deQueue(frame);
        if(!isPlaying){
            break;
        }
        if(!ret){
            continue;
        }
        //转换
        // pcm的处理逻辑
        // 音频播放器的数据格式是我们自己在下面定义的
        // 而原始数据（待播放的音频pcm数据）
        // TODO 重采样工作
        // 返回的结果：每个通道输出的样本数(注意：是转换后的)    做一个简单的重采样实验(通道基本上都是:1024)
        int samples_per_channel = swr_convert(swrContext,
                // 下面是输出区域
                                              &out_buffer,// 【成果的buff】  重采样后的
                                              out_buffers_size,// 【成果的 单通道的样本数 无法与out_buffers对应，所以有下面的pcm_data_size计算】

                // 下面是输入区域
                                              (const uint8_t **) frame->data,// 队列的AVFrame * 那的  PCM数据 未重采样的
                                              frame->nb_samples); // 输入的样本数

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
    ret = (*engineEngine)->CreateAudioPlayer(engineEngine,// 参数1：引擎接口
                                             &pcmPlayerObject, // 参数2：播放器
                                             &slDataSource, // 参数3：音频配置信息
                                             &audioSnk, // 参数4：混音器
                                             //下面代码都是 打开队列的工作
                                             1,  // 参数5：开放的参数的个数
                                             ids,// 参数6：代表我们需要 Buff
                                             req); // 参数7：代表我们上面的Buff 需要开放出去
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

    _OpenSLESStart();
    return ret;
}

void AudioChannel::_OpenSLESStart() {
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

