//
//  Java_joaobapt_FacebookManager.cpp
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

enum class PermissionState { UNKNOWN, ERROR, DECLINED, ACCEPTED };

extern "C" JNIEXPORT void JNICALL Java_joaobapt_FacebookManager_doLoginCallback(JNIEnv* env, jobject thiz, jobject callback, jlong state, jstring errorMessage);
extern "C" JNIEXPORT void JNICALL Java_joaobapt_DirectorTracker_onCurrentAccessTokenChanged(JNIEnv* env, jobject thiz, jobject oldT, jobject newT);

extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeEmptyValue(JNIEnv* env, jobject thiz);
extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeEmptyValueVector(JNIEnv* env, jobject thiz);
extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeEmptyValueMap(JNIEnv* env, jobject thiz);
extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueBoolean(JNIEnv* env, jobject thiz, jboolean val);
extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueInt(JNIEnv* env, jobject thiz, jint val);
extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueFloat(JNIEnv* env, jobject thiz, jfloat val);
extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueDouble(JNIEnv* env, jobject thiz, jdouble val);
extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueString(JNIEnv* env, jobject thiz, jstring val);

extern "C" JNIEXPORT void JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueVectorAdd(JNIEnv* env, jobject thiz, jobject vec, jobject val);
extern "C" JNIEXPORT void JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueMapAdd(JNIEnv* env, jobject thiz, jobject map, jstring key, jobject val);
extern "C" JNIEXPORT void JNICALL Java_joaobapt_RedirectToNativeCallback_doCallback(JNIEnv* env, jobject thiz, jobject value, jstring str);

JNIEXPORT void JNICALL Java_joaobapt_FacebookManager_doLoginCallback(JNIEnv* env, jobject thiz, jobject callback, jlong state, jstring errorMessage)
{
    std::function<void(PermissionState, std::string)> *result;
    result = reinterpret_cast<std::function<void(PermissionState, std::string)>*>(env->GetDirectBufferAddress(callback));
    (*result)(PermissionState(state), JniHelper::jstring2string(errorMessage));
    delete result;
}

JNIEXPORT void JNICALL Java_joaobapt_DirectorTracker_onCurrentAccessTokenChanged(JNIEnv* env, jobject thiz, jobject oldT, jobject newT)
{
    bool loggedIn = newT != nullptr;
    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("FacebookUpdated", &loggedIn);
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeEmptyValue(JNIEnv* env, jobject thiz)
{
    return env->NewDirectByteBuffer((void*)(new Value()), sizeof(Value));
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeEmptyValueVector(JNIEnv* env, jobject thiz)
{
    return env->NewDirectByteBuffer((void*)(new Value(ValueVector())), sizeof(Value));
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeEmptyValueMap(JNIEnv* env, jobject thiz)
{
    return env->NewDirectByteBuffer((void*)(new Value(ValueMap())), sizeof(Value));
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueBoolean(JNIEnv* env, jobject thiz, jboolean val)
{
    return env->NewDirectByteBuffer((void*)(new Value(bool(val))), sizeof(Value));
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueInt(JNIEnv* env, jobject thiz, jint val)
{
    return env->NewDirectByteBuffer((void*)(new Value(int(val))), sizeof(Value));
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueFloat(JNIEnv* env, jobject thiz, jfloat val)
{
    return env->NewDirectByteBuffer((void*)(new Value(float(val))), sizeof(Value));
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueDouble(JNIEnv* env, jobject thiz, jdouble val)
{
    return env->NewDirectByteBuffer((void*)(new Value(double(val))), sizeof(Value));
}

JNIEXPORT jobject JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueString(JNIEnv* env, jobject thiz, jstring val)
{
    jobject result = env->NewDirectByteBuffer((void*)(new Value(JniHelper::jstring2string(val))), sizeof(Value));
    env->DeleteLocalRef(val);
    return result;
}

JNIEXPORT void JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueVectorAdd(JNIEnv* env, jobject thiz, jobject vec, jobject val)
{
    Value *vvec = reinterpret_cast<Value*>(env->GetDirectBufferAddress(vec));
    Value *vval = reinterpret_cast<Value*>(env->GetDirectBufferAddress(val));
    vvec->asValueVector().push_back(std::move(*vval));
    delete vval;
    
    env->DeleteLocalRef(val);
    env->DeleteLocalRef(vec);
}

JNIEXPORT void JNICALL Java_joaobapt_RedirectToNativeCallback_nativeValueMapAdd(JNIEnv* env, jobject thiz, jobject map, jstring key, jobject val)
{
    Value *vmap = reinterpret_cast<Value*>(env->GetDirectBufferAddress(map));
    Value *vval = reinterpret_cast<Value*>(env->GetDirectBufferAddress(val));
    vmap->asValueMap().emplace(JniHelper::jstring2string(key), std::move(*vval));
    delete vval;
    
    env->DeleteLocalRef(key);
    env->DeleteLocalRef(val);
    env->DeleteLocalRef(map);
}

JNIEXPORT void JNICALL Java_joaobapt_RedirectToNativeCallback_doCallback(JNIEnv* env, jobject thiz, jobject value, jstring errorMessage)
{
    jclass curClass = env->GetObjectClass(thiz);
    jfieldID callbackID = env->GetFieldID(curClass, "nativeCallback", "Ljava/nio/ByteBuffer;");
    jobject callbackObject = env->GetObjectField(thiz, callbackID);
    
    std::function<void(Value&&, std::string)> *result;
    result = reinterpret_cast<std::function<void(Value&&, std::string)>*>(env->GetDirectBufferAddress(callbackObject));
    
    Value *vvalue = reinterpret_cast<Value*>(env->GetDirectBufferAddress(value));
    (*result)(std::move(*vvalue), JniHelper::jstring2string(errorMessage));
    
    delete result;
    delete vvalue;
    
    env->DeleteLocalRef(value);
    env->DeleteLocalRef(errorMessage);
    env->DeleteLocalRef(curClass);
    env->DeleteLocalRef(thiz);
}

