//
//  OpenURL.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 15/11/15.
//
//

#include "OpenURL.h"
#include "cocos2d.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif

#import <Foundation/Foundation.h>

bool openURL(std::string url)
{
    NSURL *nsurl = [NSURL URLWithString:[NSString stringWithCString:url.c_str() encoding:NSUTF8StringEncoding]];
    bool result = [[UIApplication sharedApplication] openURL:nsurl];
    
    if (!result) NSLog(@"Failed to open url: %@", [nsurl description]);
    
    return result;
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include <jni.h>
#include "platform/android/jni/JniHelper.h"

using namespace cocos2d;

bool openURL(std::string url)
{
    JniMethodInfo openURLInfo;
    if (!JniHelper::getStaticMethodInfo(openURLInfo, "org/cocos2dx/cpp/AppActivity", "openURL", "(Ljava/lang/String;)Z"))
        return false;
    
    jstring str = openURLInfo.env->NewStringUTF(url.c_str());
    jboolean result = openURLInfo.env->CallStaticBooleanMethod(openURLInfo.classID, openURLInfo.methodID, str);
    openURLInfo.env->DeleteLocalRef(str);
    
    return result;
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
#include "platform/winrt/CCWinRTUtils.h"
using namespace Windows::Foundation;
using namespace Windows::System;

bool openUrl(std::string url)
{
	try
	{
		Launcher::LaunchUriAsync(ref new Uri(cocos2d::PlatformStringFromString(url)));
		return true;
	}
	catch (Platform::Exception^)
	{
		return false;
	}
}

#endif