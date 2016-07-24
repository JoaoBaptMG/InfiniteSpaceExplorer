//
//  Java_joaobapt_CommonAlertListener.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 22/04/15.
//
//

#include <jni.h>
#include <functional>
#include "cocos2d.h"
#include "platform/android/jni/JniHelper.h"

extern "C" JNIEXPORT void JNICALL Java_joaobapt_CommonAlertListener_onClick(JNIEnv* env, jobject thiz, jobject dummy, jint id);

JNIEXPORT void JNICALL Java_joaobapt_CommonAlertListener_onClick(JNIEnv* env, jobject thiz, jobject dummy, jint id)
{
    jclass curClass = env->FindClass("joaobapt/CommonAlertListener");
    
    jfieldID confirmCallbackID = env->GetFieldID(curClass, "confirmCallback", "Ljava/nio/ByteBuffer;");
    jfieldID cancelCallbackID = env->GetFieldID(curClass, "cancelCallback", "Ljava/nio/ByteBuffer;");
    
    if (!confirmCallbackID || !cancelCallbackID) return;
    
    jobject confirmCallbackObject = env->GetObjectField(thiz, confirmCallbackID);
    jobject cancelCallbackObject = env->GetObjectField(thiz, cancelCallbackID);
    
    switch (id)
    {
        case -1:
        {
            std::function<void()> *result = reinterpret_cast<std::function<void()>*>(env->GetDirectBufferAddress(confirmCallbackObject));
            (*result)(); break;
        }
        case -2:
        {
            std::function<void()> *result = reinterpret_cast<std::function<void()>*>(env->GetDirectBufferAddress(cancelCallbackObject));
            (*result)(); break;
        }
        default: break;
    }
    
    delete reinterpret_cast<std::function<void()>*>(env->GetDirectBufferAddress(confirmCallbackObject));
    delete reinterpret_cast<std::function<void()>*>(env->GetDirectBufferAddress(cancelCallbackObject));
    
    env->DeleteLocalRef(curClass);
    env->DeleteLocalRef(thiz);
}