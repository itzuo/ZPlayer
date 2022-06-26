//
// Created by zuo on 2022/6/15/015.
//

#include "ZPlayer.h"

ZPlayer::ZPlayer(JavaCallHelper *callHelper) :callHelper(callHelper){
    //初始化网络，不调用这个，FFmpage是无法联网的
    avformat_network_init();
    pthread_mutex_init(&seekMutex, nullptr);
}

ZPlayer::~ZPlayer() {
    avformat_network_deinit();
    pthread_mutex_destroy(&seekMutex);
    //释放
//    delete this->dataSource;
//    this->dataSource = nullptr;
    DELETE(dataSource);
    //    DELETE(callHelper);//由于子线程问题，不能在这里DELETE(callHelper)
}

void ZPlayer::setDataSource(const char *_dataSource) {
    //这样写会报错，dataSource会在native-lib.cpp里的方法里会被释放掉，那么这里拿到的dataSource是悬空指针
//        this->dataSource = const_cast<char *>(dataSource);
    //strlen获得字符串长度不包括\0，所以这里要加1
    this->dataSource = new char[strlen(_dataSource)+1];
    strcpy(this->dataSource, _dataSource);
}

// void* (*__start_routine)(void*)
void *task_prepare(void *args){
    LOGE("==task_prepare==");
    ZPlayer *player = static_cast<ZPlayer *>(args);
    player->_prepare();
    return nullptr;
}

void ZPlayer::prepare() {
    LOGE("ZPlayer::prepare");
    pthread_create(&prepareTask, nullptr, task_prepare, this);
}

void ZPlayer::_prepare() {
    LOGE("==_prepare==:");
    //AVFormatContext 包含了视频的信息(宽、高等)
    avFormatContext = avformat_alloc_context();

    AVDictionary *options = nullptr;
    //设置超时时间 单位是微妙 超时时间5秒
    av_dict_set(&options, "timeout", "5000000", 0); // 单位微妙

    //1、打开媒体地址(文件地址、直播地址)， 第3个参数： 指示打开的媒体格式(传NULL，ffmpeg就会自动推导出是mp4还是flv)
    // int ret = avformat_open_input(&avFormatContext, dataSource, 0, &options);
    int ret = avformat_open_input(&avFormatContext, dataSource, 0, 0);
    av_dict_free(&options);

    //ret不为0表示打开媒体失败
    if (ret) {
        LOGE("打开媒体失败:%s", av_err2str(ret));
        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL,av_err2str(ret));
        return;
    }

    //2、查找媒体中的音视频流的信息
    ret = avformat_find_stream_info(avFormatContext, 0);
    //小于0则失败
    if (ret < 0) {
        // char * errorInfo = av_err2str(r); // 根据你的返回值 得到错误详情
        LOGE("查找流失败:%s", av_err2str(ret));
        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS, av_err2str(ret));
        return;
    }

    // 需要在avformat_find_stream_info后获取视频时长，否则.mp4可以获取到，但是.flv获取不到
    // 视频时长（单位：微妙us，转换为秒需要除以时间基AV_TIME_BASE=1000000）
    duration = avFormatContext->duration / AV_TIME_BASE;

    //经过avformat_find_stream_info方法后，formatContext->nb_streams就有值了
    unsigned int streams = avFormatContext->nb_streams;

    //nb_streams :几个流(几段视频/音频)
    for (int stream_index = 0; stream_index < streams; ++stream_index) {
        //可能代表是一个视频，也可能代表是一个音频
        AVStream *stream = avFormatContext->streams[stream_index];
        //包含了解码 这段流的各种参数信息(宽、高、码率、帧率)
        AVCodecParameters *codecpar = stream->codecpar;

        //无论视频还是音频都需要干的一些事情（获得解码器）
        // 1、通过当前流使用的编码方式，查找解码器
        AVCodec *avCodec = avcodec_find_decoder(codecpar->codec_id);
        if (avCodec == nullptr) {
            LOGE("查找解码器失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL,av_err2str(ret));
            return;
        }

        //2、获得解码器上下文
        AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
        if (avCodecContext == nullptr) {
            LOGE("创建解码上下文失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL,av_err2str(ret));
            return;
        }

        //3、设置上下文内的一些参数 (context->width)
        ret = avcodec_parameters_to_context(avCodecContext, codecpar);
        if (ret < 0) {
            LOGE("设置解码上下文参数失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL,av_err2str(ret));
            return;
        }

        // 4、打开解码器
        ret = avcodec_open2(avCodecContext, avCodec, 0);
        if (ret != 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL,av_err2str(ret));
            return;
        }

        AVRational timeBase = stream->time_base;

        //音频
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(stream_index,avCodecContext,timeBase);
            if(duration != 0){
                // 非直播
                audioChannel->setJavaCallHelper(callHelper);
            }
        }else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 虽然是视频类型，但是只有一帧封面，这个不应该参与 视频 解码 和 播放，而是跳过你
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                continue; // 过滤 封面流
            }

            //帧率： 单位时间内 需要显示多少个图像
            double fps = av_q2d(stream->avg_frame_rate);
            if(isnan(fps) || fps == 0){
                fps = av_q2d(stream->r_frame_rate);
            }
            if(isnan(fps) || fps == 0){
                fps = av_q2d(av_guess_frame_rate(avFormatContext,stream,0));
            }

            LOGE("==stream_index==%d,avCodecContext=%p",stream_index,avCodecContext);
            videoChannel = new VideoChannel(stream_index,avCodecContext,timeBase,fps);
//            videoChannel = new VideoChannel(stream_index, callHelper, avCodecContext, stream->time_base, fps);
            videoChannel->setWindow(window);
        }
    }

    //如果媒体文件中没有音视频
    if (!audioChannel && !videoChannel) {
        LOGE("没有音视频");
        callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA,av_err2str(ret));
        return;
    }
    LOGE("准备完了通知java 你随时可以开始播放");
    // 准备完了 通知java 你随时可以开始播放
    callHelper->onPrepare(THREAD_CHILD);
}

void *start_t(void *args){
    LOGE("==start_t==");
    ZPlayer *player = static_cast<ZPlayer *>(args);
    player->_start();
    return nullptr;
}


void ZPlayer::start() {
    LOGE("ZPlayer::start");
    //1、读取媒体源的数据
    //2、根据数据类型放入Audio/VideoChannel的队列中
    isPlaying = true;
    if(videoChannel){
        //音视频同步：因为要在videoChannel里获取到audioChannel里到时间戳
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->play();
    }
    if(audioChannel){
        audioChannel->play();
    }
    pthread_create(&startTask, nullptr,start_t,this);
}

/**
 * 把视频和音频的压缩包（AVPacket *） 循环获取出来 加入到队列里面去
 */
void ZPlayer::_start() {
    LOGE("_start被调用 %d",isPlaying);
    //1、读取媒体数据包(音视频数据包)
    while (isPlaying){
        // 内存泄漏关键点（控制packet队列大小，等待队列中的数据被消费）
        // 这段代码不能写在av_read_frame之后，因为会出现丢包问题
        if(videoChannel && videoChannel->packetsQueue.size() > 100){
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }
        if(audioChannel && audioChannel->packetsQueue.size() > 100){
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        // @return 0 if OK, < 0 on error or end of file
        int ret = av_read_frame(avFormatContext,packet);

        if(ret == 0){
            if(videoChannel && videoChannel->streamIndex == packet->stream_index){
                videoChannel->packetsQueue.enQueue(packet);
            }if(audioChannel && audioChannel->streamIndex == packet->stream_index){
                audioChannel->packetsQueue.enQueue(packet);
            }
        }else if(ret == AVERROR_EOF){
            //读取完成 但是可能还没播放完
            //内存泄漏点,表示读完了，要考虑释放播放完成，表示读完了 并不代表播放完毕
            if(videoChannel->packetsQueue.empty() && audioChannel->packetsQueue.empty()){
                break;// 队列的数据被音频视频全部播放完毕了
            }
        }else{
            // av_read_frame(formatContext,  packet); 出现了错误，结束当前循环
            LOGE("读取数据包失败，返回:%d 错误描述:%s", ret, av_err2str(ret));
            break;
        }
    }

    isPlaying = false;
    videoChannel->stop();
    audioChannel->stop();
}

void ZPlayer::stop() {

}

void ZPlayer::release() {

}

void ZPlayer::setWindow(ANativeWindow *window) {
    this->window = window;
    if (videoChannel) {
        videoChannel->setWindow(window);
    }
}

int ZPlayer::getDuration() {
    return duration;
}

void ZPlayer::seek(jint playValue) {
    //进去必须 在0- duration 范围之类
    if(playValue < 0 || playValue >= duration){
        return ;
    }

    if(!audioChannel && !videoChannel){
        return;
    }
    if(!avFormatContext){
        return;
    }
    pthread_mutex_lock(&seekMutex);
    //单位是 微妙
    int64_t seek = playValue * AV_TIME_BASE;
    /**
     * 1. avFormatContext 安全问题
     * 2. -1 代表默认情况，FFmpeg自动选择音频还是视频做seek操作，
     * 3.   AVSEEK_FLAG_ANY (老实)直接精准到拖动的位置，问题：如果不是关键帧，B帧 可能回造成花屏情况
     *      AVSEEK_FLAG_BACKWARD (则优， 例如：拖到8的位置是B帧，则找附件的关键帧6，如果找不到它会花屏)
     *      AVSEEK_FLAG_FRAME 找关键帧(非常不准确，可能会跳的太多)，一般不会直接用，但是会配合用
     */
     //(AVSEEK_FLAG_FRAME ｜ AVSEEK_FLAG_BACKWARD)组合使用，会有个缺陷，就是很慢
    int ret = av_seek_frame(avFormatContext, -1,seek, AVSEEK_FLAG_BACKWARD);
    //avformat_seek_file(formatContext, -1, INT64_MIN, seek, INT64_MAX, 0);
    if(ret < 0){
        return;
    }

    // 音视频正在播放，用户去seek了，是不是应该停掉播放的数据？
    // 四个队列还在工作，让它们停下来，seek完成后，重新播放。
    if(audioChannel){
        //暂停队列
        audioChannel->stopWork();
        // 清空队列
        audioChannel->clear();
        //启动队列
        audioChannel->startWork();
    }
    if(videoChannel){
        //暂停队列
        videoChannel->stopWork();
        // 清空队列
        videoChannel->clear();
        //启动队列
        videoChannel->startWork();
    }

    // 音频、与视频队列中的数据 是不是就可以丢掉了？
    pthread_mutex_unlock(&seekMutex);
}

