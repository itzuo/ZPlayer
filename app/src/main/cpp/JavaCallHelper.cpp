//
// Created by zuo on 2022/6/15/015.
//

#include "JavaCallHelper.h"
#define LOG_TAG "JavaCallHelper"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instace) {
    this->vm = vm;
    //如果在主线程回调
    this->env = env;
    //一旦涉及到jobject 跨方法/跨线程 就需要创建全局引用
    this->instace = env->NewGlobalRef(instace);

    jclass clazz = env->GetObjectClass(instace);
    onErrorId = env->GetMethodID(clazz, "onError", "(ILjava/lang/String;)V");
    onPrepareId = env->GetMethodID(clazz, "onPrepare", "()V");
    onProgressId = env->GetMethodID(clazz, "onProgress", "(I)V");
}

JavaCallHelper::~JavaCallHelper() {
    env->DeleteGlobalRef(this->instace);
}

void JavaCallHelper::onError(int thread, int errorCode,char * ffmpegError) {
    //主线程
    if(thread == THREAD_MAIN){
        jstring _ffmpegError = env->NewStringUTF(ffmpegError);
        env->CallVoidMethod(instace,onErrorId,errorCode,_ffmpegError);
    } else{
        //子线程
        JNIEnv *env;
        if (vm->AttachCurrentThread(&env, 0) != JNI_OK) {
            return;
        }
        jstring _ffmpegError = env->NewStringUTF(ffmpegError);
        env->CallVoidMethod(instace,onErrorId,errorCode,_ffmpegError);
        vm->DetachCurrentThread();//解除附加，必须要
    }
}

void JavaCallHelper::onPrepare(int thread) {
    //主线程
    if(thread == THREAD_MAIN){
        env->CallVoidMethod(instace,onPrepareId);
    } else{
        //子线程
        JNIEnv *env;
        if (vm->AttachCurrentThread(&env, 0) != JNI_OK) {
            return;
        }
        env->CallVoidMethod(instace,onPrepareId);
        vm->DetachCurrentThread();
    }
}

void JavaCallHelper::onProgress(int thread, int progress) {
    if(thread == THREAD_MAIN){
        env->CallVoidMethod(instace,onProgressId,progress);
    } else{
        //子线程
        JNIEnv *env;
        if (vm->AttachCurrentThread(&env, 0) != JNI_OK) {
            return;
        }
        env->CallVoidMethod(instace,onProgressId,progress);
        vm->DetachCurrentThread();
    }
}
