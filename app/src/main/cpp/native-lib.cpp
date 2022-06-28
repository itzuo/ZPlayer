#include <jni.h>
#include <string>
#include "ZPlayer.h"

ZPlayer *zPlayer = nullptr;
JavaVM *javaVm = nullptr;
JavaCallHelper *helper = nullptr;
ANativeWindow *window = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int JNI_OnLoad(JavaVM *vm,void *r){
    javaVm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeInit(JNIEnv *env, jobject thiz) {
    //创建播放器
    helper = new JavaCallHelper(javaVm,env,thiz);
    zPlayer = new ZPlayer(helper);
    return (jlong)zPlayer;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_setDataSource(JNIEnv *env, jobject thiz, jlong native_handle,
                                           jstring path) {
    const char *dataSource = env->GetStringUTFChars(path, nullptr);
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    zPlayer->setDataSource(dataSource);
    env->ReleaseStringUTFChars(path,dataSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativePrepare(JNIEnv *env, jobject thiz, jlong native_handle) {
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    zPlayer->prepare();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeStart(JNIEnv *env, jobject thiz, jlong native_handle) {
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    zPlayer->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeStop(JNIEnv *env, jobject thiz, jlong native_handle) {
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    zPlayer->stop();
    if(helper){
        DELETE(helper);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeRelease(JNIEnv *env, jobject thiz, jlong native_handle) {
    /*ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    if(zPlayer){
        zPlayer->release();
    }*/
    pthread_mutex_lock(&mutex);
    if(window){
        ANativeWindow_release(window);
        window = nullptr;
    }
    pthread_mutex_unlock(&mutex);
    DELETE(helper);
    DELETE(zPlayer);
    DELETE(javaVm);
    DELETE(window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeSetSurface(JNIEnv *env, jobject thiz, jlong native_handle,
                                              jobject surface) {
    pthread_mutex_lock(&mutex);
    //先释放之前的显示窗口
    if(window){
        LOGE("nativeSetSurface->window=%p",window);
        ANativeWindow_release(window);
        window = nullptr;
    }
    window = ANativeWindow_fromSurface(env,surface);
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    zPlayer->setWindow(window);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_zxj_zplayer_ZPlayer_getNativeDuration(JNIEnv *env, jobject thiz, jlong native_handle) {
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    if(zPlayer){
        return zPlayer->getDuration();
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeSeek(JNIEnv *env, jobject thiz, jint play_value,
                                        jlong native_handle) {
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(native_handle);
    if(zPlayer){
        zPlayer->seek(play_value);
    }
}