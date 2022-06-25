//
// Created by zuo on 2022/6/15/015.
//

#include "VideoChannel.h"

VideoChannel::VideoChannel(int streamIndex, AVCodecContext *codecContext) : BaseChannel(streamIndex,
                                                                                        codecContext) {
    LOGE("=VideoChannel=streamIndex==%d", streamIndex);
    pthread_mutex_init(&surfaceMutex, 0);
}

VideoChannel::~VideoChannel() {
    pthread_mutex_destroy(&surfaceMutex);
}


void *videoDecode_t(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decode();
    return nullptr;
}

void *videoPlay_t(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->_play();
    return nullptr;
}

void VideoChannel::play() {
    isPlaying = true;
    // 开启队列的工作
    setEnable(true);
    //解码
    pthread_create(&videoDecodeTask, nullptr, videoDecode_t, this);
    //播放
    pthread_create(&videoPlayTask, nullptr, videoPlay_t, this);
}

/**
 * 取出队列的压缩包(AVPacket *),进行解码,解码后的原始包(AVFrame *)再push队列中去
 */
void VideoChannel::decode() {
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
//        releaseAVPacket(&packet);

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
            // 内存泄漏点,解码视频的frame出错，马上释放，防止你在堆区开辟了空间
            if(frame){
                releaseAVFrame(&frame);
            }
            break;
        }

        //再开一个线程 来播放 (流畅度)
        frameQueue.enQueue(frame);// 加入队列--YUV数据
        av_packet_unref(packet);
        releaseAVPacket(&packet);
    }
    av_packet_unref(packet);
    releaseAVPacket(&packet);
}

/**
 * 从队列取出原始包(AVFrame *)进行播放
 */
void VideoChannel::_play() {
    // 缩放、格式转换
    SwsContext *swsContext = sws_getContext(
            // 下面是输入环节
            avCodecContext->width,
            avCodecContext->height,
            avCodecContext->pix_fmt,

            // 下面是输出环节
            avCodecContext->width,
            avCodecContext->height,
            AV_PIX_FMT_RGBA,

            SWS_FAST_BILINEAR, NULL, NULL, NULL);

    //指针数组
    uint8_t *dst_data[4];
    int dst_linesize[4];
    av_image_alloc(dst_data, dst_linesize, avCodecContext->width, avCodecContext->height,
                   AV_PIX_FMT_RGBA, 1);

//    double frame_delay =1.0 /fps;//标准延迟时间

    int ret;
    AVFrame *frame = 0;
    while (isPlaying) {

        //阻塞方法
        ret = frameQueue.deQueue(frame);
        // 阻塞完了之后可能用户已经停止播放
        if (!isPlaying) {
            break;
        }

        if (!ret) {
            continue;
        }

        //todo 参数2、指针数据，比如RGBA，每一个维度的数据就是一个指针，那么RGBA需要4个指针，所以就是4个元素的数组，数组的元素就是指针，指针数据
        // 参数3、每一行数据他的数据个数
        // 参数4、 offset
        // 参数5、 要转化图像的高
        sws_scale(swsContext,
                  // 下面是输入环节  YUV的数据
                  frame->data, frame->linesize, 0, frame->height,

                  // 下面是输出环节 成果：RGBA数据 Android SurfaceView播放画面
                  dst_data,
                  dst_linesize);

        //画画
        _onDraw(dst_data[0], dst_linesize[0], avCodecContext->width, avCodecContext->height);
        releaseAVFrame(&frame);// 释放原始包，因为已经被渲染了，没用了
    }
    av_free(&dst_data[0]);
    isPlaying = 0;
    releaseAVFrame(&frame);
    sws_freeContext(swsContext);
}

void VideoChannel::stop() {
    isPlaying = false;
    setEnable(false);
    pthread_join(videoDecodeTask, 0);
    pthread_join(videoPlayTask, 0);
}

void VideoChannel::setWindow(ANativeWindow *window) {
    LOGE("VideoChannel->setWindow=%p", window);
    pthread_mutex_lock(&surfaceMutex);
    if (this->window) {
        ANativeWindow_release(this->window);
    }
    this->window = window;
    pthread_mutex_unlock(&surfaceMutex);
}

void VideoChannel::_onDraw(uint8_t *dst_data, int dst_linesize, int width, int height){
    pthread_mutex_lock(&surfaceMutex);
    if (!window) {
        pthread_mutex_unlock(&surfaceMutex);
        return;
    }

    // 设置窗口的大小，各个属性
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    // 他自己有个缓冲区 buffer
    ANativeWindow_Buffer buffer;//要显示到window上面到缓存数据

    //ANativeWindow_lock 将buffer与window进行绑定
    if (ANativeWindow_lock(window, &buffer, 0) != 0) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&surfaceMutex);
        return;
    }

    // 开始真正的渲染，因为window没有被锁住了，就可以rgba数据 ----> 字节对齐 渲染
    //把视频数据刷到buffer中
    uint8_t *dstData = static_cast<uint8_t *>(buffer.bits);
    int dstLineSize = buffer.stride * 4;
    // 视频图像的rgba数据
//    uint8_t *srcData = dst_data[0];
//    int srcSize = dst_linesize[0];

    //一行一行拷贝
    for (int i = 0; i < buffer.height; ++i) {
//        LOGE("dstLineSize=%d,dst_linesize=%d",dstLineSize,dst_linesize);
        memcpy(dstData + i * dstLineSize, dst_data + i * dst_linesize, dst_linesize);
//        memcpy(dstData + i * dstLineSize, dst_data + i * dst_linesize, dstLineSize);
    }

    ANativeWindow_unlockAndPost(window);//解绑并提交
    pthread_mutex_unlock(&surfaceMutex);
}



