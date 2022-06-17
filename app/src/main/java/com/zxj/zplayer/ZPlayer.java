package com.zxj.zplayer;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;

/**
 * @author zuo
 * @date 2022/6/15/015 14:33
 */
public class ZPlayer implements LifecycleObserver, SurfaceHolder.Callback {
    private static final String TAG = ZPlayer.class.getSimpleName();

    static {
        System.loadLibrary("z-player");
    }

    private final long nativeHandle;
    private OnPrepareListener listener;
    private OnErrorListener onErrorListener;
    private SurfaceHolder mHolder;

    public ZPlayer() {
        nativeHandle = nativeInit();
    }

    /**
     * 设置播放显示的画布
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView) {
        if (this.mHolder != null) {
            mHolder.removeCallback(this); // 清除上一次的
        }
        mHolder = surfaceView.getHolder();
        mHolder.addCallback(this);
    }

    /**
     * 让使用者设置播放的文件，或者直播地址
     * @param dataSource
     */
    public void setDataSource(String dataSource){
        setDataSource(nativeHandle, dataSource);
    }

    /**
     * 准备好要播放的视频
     */
    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    public void prepare() {
        Log.e(TAG,"ZPlayer->prepare");
        nativePrepare(nativeHandle);
    }

    /**
     * 开始播放
     */
    public void start(){
        Log.e(TAG,"ZPlayer->start");
        nativeStart(nativeHandle);
    }

    /**
     * 停止播放
     */
    @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
    public void stop(){
        Log.e(TAG,"ZPlayer->stop");
        nativeStop(nativeHandle);
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    public void release(){
        Log.e(TAG,"ZPlayer->release");
        mHolder.removeCallback(this);
        nativeRelease(nativeHandle);
    }

    /**
     * JavaCallHelper 会反射调用此方法
     * @param errorCode
     */
    public void onError(int errorCode, String ffmpegError){
        Log.e(TAG,"Java接收到回调了->onError:"+errorCode);
        String title = "\nFFmpeg给出的错误如下:\n";

        String msg = null;
        switch (errorCode){
            case Constant.FFMPEG_CAN_NOT_OPEN_URL:
                msg = "打不开视频"+title+ ffmpegError;
                break;
            case Constant.FFMPEG_CAN_NOT_FIND_STREAMS:
                msg = "找不到流媒体"+title+ ffmpegError;
                break;
            case Constant.FFMPEG_FIND_DECODER_FAIL:
                msg = "找不到解码器"+title+ ffmpegError;
                break;
            case Constant.FFMPEG_ALLOC_CODEC_CONTEXT_FAIL:
                msg = "无法根据解码器创建上下文"+title+ ffmpegError;
                break;
            case Constant.FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL:
                msg = "根据流信息 配置上下文参数失败"+title+ ffmpegError;
                break;
            case Constant.FFMPEG_OPEN_DECODER_FAIL:
                msg = "打开解码器失败"+title+ ffmpegError;
                break;
            case Constant.FFMPEG_NOMEDIA:
                msg = "没有音视频"+title+ ffmpegError;
                break;
        }
        if(onErrorListener != null ){
            onErrorListener.onError(msg);
        }
    }

    /**
     * JavaCallHelper 会反射调用此方法
     */
    public void onPrepare(){
        Log.e(TAG,"Java接收到回调了->onPrepare");
        if(listener != null){
            listener.onPrepare();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        Log.e(TAG,"ZPlayer->surfaceCreated");
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        Log.e(TAG,"ZPlayer->surfaceChanged");
        nativeSetSurface(nativeHandle,surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        Log.e(TAG,"ZPlayer->surfaceDestroyed");
    }

    public interface OnPrepareListener{
        void onPrepare();
    }

    public void setOnPrepareListener(OnPrepareListener listener){
        this.listener = listener;
    }

    public interface OnErrorListener{
        void onError(String errorCode);
    }

    public void setOnErrorListener(OnErrorListener listener){
        this.onErrorListener = listener;
    }

    private native long nativeInit();
    private native void setDataSource(long nativeHandle, String path);
    private  native void nativePrepare(long nativeHandle);
    private native void nativeStart(long nativeHandle);
    private native void nativeStop(long nativeHandle);
    private  native void nativeRelease(long nativeHandle);
    private native void nativeSetSurface(long nativeHandle, Surface surface);
}
