#include <jni.h>
#include <string>
#include "ZPlayer.h"

ZPlayer *zPlayer = nullptr;
JavaVM *javaVm = nullptr;
JavaCallHelper *helper = nullptr;

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
Java_com_zxj_zplayer_ZPlayer_setDataSource(JNIEnv *env, jobject thiz, jlong nativeHandle,
                                           jstring path) {
    const char *dataSource = env->GetStringUTFChars(path, nullptr);
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(nativeHandle);
    zPlayer->setDataSource(dataSource);
    env->ReleaseStringUTFChars(path,dataSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativePrepare(JNIEnv *env, jobject thiz, jlong nativeHandle) {
    ZPlayer *zPlayer = reinterpret_cast<ZPlayer *>(nativeHandle);
    zPlayer->prepare();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeStart(JNIEnv *env, jobject thiz, jlong native_handle) {


}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeStop(JNIEnv *env, jobject thiz, jlong native_handle) {


}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeRelease(JNIEnv *env, jobject thiz, jlong native_handle) {


}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativeSetSurface(JNIEnv *env, jobject thiz, jlong native_handle,
                                              jobject surface) {


}