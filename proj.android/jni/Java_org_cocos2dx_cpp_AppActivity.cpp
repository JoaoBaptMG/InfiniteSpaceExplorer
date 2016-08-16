//
//  Java_org_cocos2dx_cpp_AppActivity.cpp
//  
//
//  Created by João Baptista on 15/08/16.
//
//

#include <jni.h>
#include <gpg/gpg.h>

extern "C" JNIEXPORT void JNICALL Java_org_cocos2dx_cpp_AppActivity_gpgOnActivityResult(JNIEnv *env, jobject thiz, jobject activity, jint request_code, jint result_code, jobject data);

JNIEXPORT void JNICALL Java_org_cocos2dx_cpp_AppActivity_gpgOnActivityResult(JNIEnv *env, jobject thiz, jobject activity, jint request_code, jint result_code, jobject data)
{
	gpg::AndroidSupport::OnActivityResult(env, activity, request_code, result_code, data);
}