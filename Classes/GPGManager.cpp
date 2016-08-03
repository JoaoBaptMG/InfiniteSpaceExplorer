//
//  GPGManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 24/07/16.
//
//

#include "GPGManager.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include <jni.h>
#include <string>
#include "platform/android/jni/JniHelper.h"
#endif

#include <gpg/gpg.h>

using namespace cocos2d;

static std::unique_ptr<gpg::GameServices> gameServices;
static bool gpgActive = false;
static std::string errorString;

void GPGManager::initialize(std::function<void()> success)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
	gpg::IosPlatformConfiguration config;
	config.SetClientID("TODO_CONFIG_NAME");
	config.SetOptionalViewControllerForPopups([[UIApplication sharedApplication].keyWindow.rootViewController);
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	JniMethodInfo getActivityInfo;
	auto loaded = JniHelper::getStaticMethodInfo(getActivityInfo, "org/cocos2dx/cpp/AppActivity", "getActivity", "()Landroid/app/Activity");
	CC_ASSERT(loaded);

	jobject activity = getActivityInfo.env->CallStaticObjectMethod(getActivityInfo.classID, getActivityInfo.methodID);
	getActivityInfo.env->DeleteLocalRef(getActivityInfo.classID);

	gpg::AndroidPlatformConfiguration config;
	config.SetActivity(activity);
#endif

	bool uiLaunched = false;
	auto onAuthFinished = [=](gpg::AuthOperation op, gpg::AuthStatus status) mutable
	{
		if (op == gpg::AuthOperation::SIGN_IN)
		{
			if (gpg::IsSuccess(status))
			{
				success(); 
				gpgActive = true;
			}
		}
		else gpgActive = uiLaunched = false;
	};

	gameServices = gpg::GameServices::Builder().SetOnAuthActionFinished(onAuthFinished).Create(config);
}

#endif