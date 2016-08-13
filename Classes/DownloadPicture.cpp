//
//  DownloadPicture.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 19/09/15.
//
//

#include "DownloadPicture.h"

using namespace cocos2d;

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif

#import <Foundation/Foundation.h>

void downloadPicture(std::string path, std::string key, std::function<void(cocos2d::Texture2D*)> callback)
{
    NSURL *url = [NSURL URLWithString:[NSString stringWithCString:path.c_str() encoding:NSUTF8StringEncoding]];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0ul), ^
    {
        NSData *data = [NSData dataWithContentsOfURL:url];
        Image *image = new Image();
        image->initWithImageData(reinterpret_cast<const unsigned char*>(data.bytes), data.length);
        image->retain();
        
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]
        {
            callback(Director::getInstance()->getTextureCache()->addImage(image, key));
            image->release();
        });
    });
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "platform/android/jni/JniHelper.h"

void downloadPicture(std::string path, std::string key, std::function<void(cocos2d::Texture2D*)> callback)
{
    JniMethodInfo asyncTaskCtr, asyncTaskExecute, urlCtr;
    
    if (!JniHelper::getMethodInfo(asyncTaskCtr, "joaobapt.PictureDownloader", "<init>", "(Ljava/nio/ByteBuffer;)V")) return;
    if (!JniHelper::getMethodInfo(asyncTaskExecute, "joaobapt.PictureDownloader", "execute", "([Ljava/lang/Object;)Landroid/os/AsyncTask;")) return;
    if (!JniHelper::getMethodInfo(urlCtr, "java.net.URL", "<init>", "(Ljava/lang/String;)V")) return;
    
    auto callbackFunction = [=] (const unsigned char* data, long length, std::string error)
    {
        if (length > 0)
        {
            Image *image = new Image();
            image->initWithImageData(data, length);
            image->retain();
            
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]
            {
                callback(Director::getInstance()->getTextureCache()->addImage(image, key));
                image->release();
            });
        }
    };
    
    jstring pathStr = urlCtr.env->NewStringUTF(path.c_str());
    jobject callbackObj = asyncTaskExecute.env->NewDirectByteBuffer((void*)(new (std::nothrow) std::function<void(const unsigned char*, long, std::string)>(callbackFunction)),
                                                                    sizeof(std::function<void(const unsigned char*, long, std::string)>));
    
    jobject url = urlCtr.env->NewObject(urlCtr.classID, urlCtr.methodID, pathStr);
    jobjectArray urlArray = urlCtr.env->NewObjectArray(1, urlCtr.classID, url);
    
    jobject asyncTask = asyncTaskCtr.env->NewObject(asyncTaskCtr.classID, asyncTaskCtr.methodID, callbackObj);
    
    asyncTaskExecute.env->CallObjectMethod(asyncTask, asyncTaskExecute.methodID, urlArray);
    
    asyncTaskCtr.env->DeleteLocalRef(asyncTask);
    urlCtr.env->DeleteLocalRef(urlArray);
    urlCtr.env->DeleteLocalRef(url);
    asyncTaskExecute.env->DeleteLocalRef(callbackObj);
    urlCtr.env->DeleteLocalRef(pathStr);
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_WINRT

#include "platform/winrt/CCWinRTUtils.h"
#include <wrl.h>
#include <robuffer.h>

using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Web::Http;
using namespace Concurrency;
using namespace Microsoft::WRL;

void downloadPicture(std::string path, std::string key, std::function<void(cocos2d::Texture2D*)> callback)
{
	try
	{
		auto httpClient = ref new HttpClient();
		auto uri = ref new Uri(PlatformStringFromString(path));

		create_task(httpClient->GetBufferAsync(uri)).then([=](IBuffer^ buffer)
		{
			if (buffer->Length > 0)
			{
				ComPtr<IBufferByteAccess> byteAccess;
				reinterpret_cast<IInspectable*>(buffer)->QueryInterface(IID_PPV_ARGS(&byteAccess));

				byte* data;
				byteAccess->Buffer(&data);

				Image *image = new Image();
				image->initWithImageData(data, buffer->Length);
				image->retain();

				Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]
				{
					callback(Director::getInstance()->getTextureCache()->addImage(image, key));
					image->release();
				});
			}
		});
	}
	catch (Platform::Exception^ e)
	{

	}
}

#endif