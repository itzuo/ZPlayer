#include <jni.h>
#include <string>
#include "ZPlayer.h"

ZPlayer *ffmpeg = nullptr;
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
    ffmpeg = new ZPlayer(helper);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_setDataSource(JNIEnv *env, jobject thiz, jlong native_handle,
                                           jstring path) {

}

extern "C"
JNIEXPORT void JNICALL
Java_com_zxj_zplayer_ZPlayer_nativePrepare(JNIEnv *env, jobject thiz, jlong native_handle) {


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