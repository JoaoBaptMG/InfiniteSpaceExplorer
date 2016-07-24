//
//  MessageDialog.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 22/04/15.
//
//

#include "MessageDialog.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif

#import <UIKit/UIAlert.h>

@interface CommonAlertDelegate : NSObject <UIAlertViewDelegate>
{
    std::function<void()> confirmCallback, cancelCallback;
}

- (instancetype)initWithCallbacksForConfirm:(std::function<void()>)confirmCallback cancel:(std::function<void()>)cancelCallback;
@end

@implementation CommonAlertDelegate
- (instancetype)initWithCallbacksForConfirm:(std::function<void()>)confirmCallback cancel:(std::function<void()>)cancelCallback
{
    if (self = [super init])
    {
        self->confirmCallback = confirmCallback;
        self->cancelCallback = cancelCallback;
    }
    
    return self;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 0) cancelCallback();
    else confirmCallback();
    
    [self release];
}
@end

#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include "platform/android/jni/JniHelper.h"
using namespace cocos2d;

#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8

#endif

void presentMessage(std::string message, std::string title, std::string confirmCaption, std::string cancelCaption, std::function<void()> confirmCallback, std::function<void()> cancelCallback)
{
    if (message.empty()) return;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    UIAlertView *view = [[UIAlertView alloc] initWithTitle:[NSString stringWithUTF8String:title.c_str()]
                                                   message:[NSString stringWithUTF8String:message.c_str()]
                                                  delegate:[[CommonAlertDelegate alloc] initWithCallbacksForConfirm:confirmCallback cancel:cancelCallback]
                                         cancelButtonTitle:[NSString stringWithUTF8String:cancelCaption.c_str()]
                                         otherButtonTitles:[NSString stringWithUTF8String:confirmCaption.c_str()], nil];
    
    [view autorelease];
    [view show];
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    JniMethodInfo t;
    if (JniHelper::getStaticMethodInfo(t, "org/cocos2dx/cpp/AppActivity", "getActivity", "()Landroid/app/Activity;"))
    {
        JniMethodInfo t1;
        if (JniHelper::getMethodInfo(t1, "joaobapt.CommonAlertListener", "<init>", "()V"))
        {
            JniMethodInfo t2;
#define METHOD_SIG "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V"
            if (JniHelper::getMethodInfo(t2, "joaobapt.CommonAlertListener", "presentDialog", METHOD_SIG))
            {
                jobject listener = t1.env->NewObject(t1.classID, t1.methodID);
                
                jobject context = t.env->CallStaticObjectMethod(t.classID, t.methodID);
                jstring titleStr = t1.env->NewStringUTF(title.c_str());
                jstring messageStr = t1.env->NewStringUTF(message.c_str());
                jstring confirmStr = t1.env->NewStringUTF(confirmCaption.c_str());
                jstring cancelStr = t1.env->NewStringUTF(cancelCaption.c_str());
                
                jobject confirmCallbackBuffer = t2.env->NewDirectByteBuffer((void*)(new (std::nothrow) std::function<void()>(confirmCallback)), sizeof(std::function<void()>));
                jobject cancelCallbackBuffer = t2.env->NewDirectByteBuffer((void*)(new (std::nothrow) std::function<void()>(cancelCallback)), sizeof(std::function<void()>));
                
                t2.env->CallVoidMethod(listener, t2.methodID, context, messageStr, titleStr, confirmStr, cancelStr, confirmCallbackBuffer, cancelCallbackBuffer);
                
                t2.env->DeleteLocalRef(confirmCallbackBuffer);
                t2.env->DeleteLocalRef(cancelCallbackBuffer);
                t1.env->DeleteLocalRef(titleStr);
                t1.env->DeleteLocalRef(messageStr);
                t1.env->DeleteLocalRef(confirmStr);
                t1.env->DeleteLocalRef(cancelStr);
            }
        }
    }
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8
    assert(false && "Not implemented!");
#endif
}