//
// Created by zuo on 2022/6/15/015.
//

#ifndef ZPLAYER_JAVACALLHELPER_H
#define ZPLAYER_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {

public:
    JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instace);
    ~JavaCallHelper();
    //回调Java
    void onError(int thread,int errorCode);
    void onPrepare(int thread);

private:
    JavaVM *vm;
    JNIEnv *env;
    jobject instace;
    jmethodID onErrorId;
    jmethodID onPrepareId;
};


#endif //ZPLAYER_JAVACALLHELPER_H
