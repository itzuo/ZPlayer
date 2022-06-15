package com.zxj.zplayer;

/**
 * @author zuo
 * @date 2022/6/15/015 17:01
 */
public interface Constant {
    //错误代码
    //打不开视频
    int FFMPEG_CAN_NOT_OPEN_URL = 1;
    //找不到流媒体
    int FFMPEG_CAN_NOT_FIND_STREAMS = 2;
    //找不到解码器
    int FFMPEG_FIND_DECODER_FAIL = 3;
    //无法根据解码器创建上下文
    int FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = 4;
    //根据流信息 配置上下文参数失败
    int FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = 6;
    //打开解码器失败
    int FFMPEG_OPEN_DECODER_FAIL = 7;
    //没有音视频
    int FFMPEG_NOMEDIA = 8;
}

