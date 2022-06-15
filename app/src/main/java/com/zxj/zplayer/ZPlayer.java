package com.zxj.zplayer;

import android.util.Log;
import android.view.Surface;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;

/**
 * @author zuo
 * @date 2022/6/15/015 14:33
 */
public class ZPlayer implements LifecycleObserver {
    private static final String TAG = ZPlayer.class.getSimpleName();

    static {
        System.loadLibrary("z-player");
    }

    private final long nativeHandle;

    public ZPlayer() {
        nativeHandle = nativeInit();
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
        Log.e(TAG,"prepare");
        nativePrepare(nativeHandle);
    }

    /**
     * 开始播放
     */
    public void start(){
        nativeStart(nativeHandle);
    }

    /**
     * 停止播放
     */
    @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
    public void stop(){
        Log.e(TAG,"stop");
        nativeStop(nativeHandle);
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    public void release(){
        Log.e(TAG,"release");
//        mHolder.removeCallback(this);
        nativeRelease(nativeHandle);
    }


    private native long nativeInit();
    private native void setDataSource(long nativeHandle, String path);
    private  native void nativePrepare(long nativeHandle);
    private native void nativeStart(long nativeHandle);
    private native void nativeStop(long nativeHandle);
    private  native void nativeRelease(long nativeHandle);
    private native void nativeSetSurface(long nativeHandle, Surface surface);
}
