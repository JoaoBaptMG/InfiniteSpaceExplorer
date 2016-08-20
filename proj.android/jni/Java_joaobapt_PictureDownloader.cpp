//
//  Java_joaobapt_PictureDownloader.cpp
//  
//
//  Created by João Baptista on 20/09/15.
//
//

#include <jni.h>
#include <functional>
#include <string>
#include "cocos2d.h"
#include "platform/android/jni/JniHelper.h"

using namespace cocos2d;

extern "C" JNIEXPORT void JNICALL Java_joaobapt_PictureDownloader_onPostExecute(JNIEnv* env, jobject thiz, jobject result);

JNIEXPORT void JNICALL Java_joaobapt_PictureDownloader_onPostExecute(JNIEnv* env, jobject thiz, jobject result)
{
    jclass curClass = env->FindClass("joaobapt/PictureDownloader");
    
	jfieldID stringMessageID = env->GetFieldID(curClass, "errorMessage", "Ljava/lang/String;");
    jfieldID callbackID = env->GetFieldID(curClass, "callback", "Ljava/nio/ByteBuffer;");
    
    if (!callbackID) return;
    
    jobject callbackObject = env->GetObjectField(thiz, callbackID);
    jstring errorMessage = (jstring)env->GetObjectField(thiz, stringMessageID);
    
    std::function<void(const unsigned char*, long, std::string)> *func;
    func = reinterpret_cast<std::function<void(const unsigned char*, long, std::string)>*>(env->GetDirectBufferAddress(callbackObject));
    
    (*func)(reinterpret_cast<const unsigned char*>(env->GetDirectBufferAddress(result)),
              env->GetDirectBufferCapacity(result), JniHelper::jstring2string(errorMessage));
    
    delete func;
    
    env->DeleteLocalRef(curClass);
    env->DeleteLocalRef(thiz);
}