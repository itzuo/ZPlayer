//
// Created by zuo on 2022/6/15/015.
//

#include "ZPlayer.h"

ZPlayer::ZPlayer(JavaCallHelper *callHelper) :callHelper(callHelper){
    //初始化网络，不调用这个，FFmpage是无法联网的
    avformat_network_init();
}

ZPlayer::~ZPlayer() {
    avformat_network_deinit();
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
    LOGE("==task_prepare==:");
    ZPlayer *player = static_cast<ZPlayer *>(args);
    player->_prepare();
    return 0;
}

void ZPlayer::prepare() {
    LOGE("ZPlayer::prepare");
    pthread_create(&prepareTask, nullptr, task_prepare, this);
    LOGE("ZPlayer::prepare==");
}

void ZPlayer::start() {

}

void ZPlayer::stop() {

}

void ZPlayer::release() {

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
        AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);
        if (codecContext == nullptr) {
            LOGE("创建解码上下文失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL,av_err2str(ret));
            return;
        }

        //3、设置上下文内的一些参数 (context->width)
        ret = avcodec_parameters_to_context(codecContext, codecpar);
        if (ret < 0) {
            LOGE("设置解码上下文参数失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL,av_err2str(ret));
            return;
        }

        // 4、打开解码器
        ret = avcodec_open2(codecContext, avCodec, 0);
        if (ret != 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL,av_err2str(ret));
            return;
        }

        //音频
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel();
        }else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //帧率： 单位时间内 需要显示多少个图像
            double fps = av_q2d(stream->avg_frame_rate);
            if(isnan(fps) || fps == 0){
                fps = av_q2d(stream->r_frame_rate);
            }
            if(isnan(fps) || fps == 0){
                fps = av_q2d(av_guess_frame_rate(avFormatContext,stream,0));
            }

            videoChannel = new VideoChannel();
//            videoChannel = new VideoChannel(stream_index, callHelper, codecContext, stream->time_base, fps);
//            videoChannel->setWindow(window);
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

void ZPlayer::_start() {

}
