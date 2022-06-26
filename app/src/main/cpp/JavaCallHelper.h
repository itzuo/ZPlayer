//
// Created by zuo on 2022/6/15/015.
//

#ifndef ZPLAYER_JAVACALLHELPER_H
#define ZPLAYER_JAVACALLHELPER_H

#include <jni.h>
#include "macro.h"

class JavaCallHelper {

public:
    JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instace);
    ~JavaCallHelper();
    //回调Java
    void onError(int thread,int errorCode,char * ffmpegError);
    void onPrepare(int thread);
    void onProgress(int thread,int progress);

private:
    JavaVM *vm  = 0;   // 只有他 才能 跨越线程
    JNIEnv *env = 0;
    jobject instace;
    jmethodID onErrorId;
    jmethodID onPrepareId;
    jmethodID onProgressId;
};


#endif //ZPLAYER_JAVACALLHELPER_H
