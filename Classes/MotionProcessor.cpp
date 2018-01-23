//
//  MotionProcessor.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 26/09/15.
//
//

#include "MotionProcessor.h"
using namespace cocos2d;

constexpr float K = 0.4;

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif

#import <Foundation/Foundation.h>
#import <CoreMotion/CoreMotion.h>

class MotionProcessoriOS : public MotionProcessor
{
    NSOperationQueue *queue;
    CMMotionManager *manager;
    
    Quaternion currentQuaternion, calibratedQuaternion;
    Vec2 directionVector;
    std::atomic<UIInterfaceOrientation> curOrientation;
    
public:
    virtual ~MotionProcessoriOS() override
    {
        [manager stopDeviceMotionUpdates];
        [queue waitUntilAllOperationsAreFinished];
    }
    
    virtual void calibrate() override
    {
        [queue addOperationWithBlock:^
         {
            calibratedQuaternion = currentQuaternion;
            calibratedQuaternion.conjugate();
            directionVector.set(0, 0);
         }];
    }
    
    virtual Vec2 getDirectionVector() override
    {
        [queue waitUntilAllOperationsAreFinished];
        
        return directionVector;
    }
    
    void update(float dt)
    {
        curOrientation = [UIApplication sharedApplication].statusBarOrientation;
    }
    
protected:
    MotionProcessoriOS() : queue([[NSOperationQueue alloc] init]), manager([[CMMotionManager alloc] init]), currentQuaternion(0, 0, 0, 0), calibratedQuaternion(0, 0, 0, 0),
                            directionVector(0, 0)
    {
        Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);
        
        manager.deviceMotionUpdateInterval = 1.0f/60;
        [manager startDeviceMotionUpdatesUsingReferenceFrame:CMAttitudeReferenceFrameXArbitraryZVertical toQueue:queue withHandler:
         ^(CMDeviceMotion *device, NSError *error)
         {
            if (device != nil)
            {
                CMQuaternion dquat = device.attitude.quaternion;
                currentQuaternion.set(dquat.x, dquat.y, dquat.z, dquat.w);
                
                //CCLOG("%g %g %g %g", currentQuaternion.x, currentQuaternion.y, currentQuaternion.z, currentQuaternion.w);
                
                Quaternion quat = currentQuaternion * calibratedQuaternion;
                
                float tanpitch = .625 * (2*quat.w*quat.y - 2*quat.x*quat.z) / (1 - 2*quat.y*quat.y - 2*quat.z*quat.z);
                float tanroll = .625 * (2*quat.w*quat.x - 2*quat.y*quat.z) / (1 - 2*quat.x*quat.x - 2*quat.z*quat.z);
                
                //log("currentValue = %g, %g", tanroll, tanpitch);
                
                Vec2 cur(tanroll, tanpitch);
                
                if (curOrientation == UIInterfaceOrientationLandscapeLeft)
                    cur.negate();
                
                directionVector = directionVector*(1-K) + cur*K;
            }
         }];
    }
    
    friend MotionProcessor *createMotionProcessor();
};

MotionProcessor *createMotionProcessor()
{
    return new MotionProcessoriOS();
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include <jni.h>
#include "platform/android/jni/JniHelper.h"

jmethodID MotionProcessor_ctor, MotionProcessor_calibrate, MotionProcessor_getDirectionVector, MotionProcessor_dispose;
static bool methodsInited = false;

class MotionProcessorAndroid : public MotionProcessor
{
    jobject motionProcessorObject;
    
public:
    virtual ~MotionProcessorAndroid() override
    {
        JNIEnv *env = JniHelper::getEnv();
        env->CallVoidMethod(motionProcessorObject, MotionProcessor_dispose);
        env->DeleteGlobalRef(motionProcessorObject);
    }
    
    virtual void calibrate() override
    {
        JNIEnv *env = JniHelper::getEnv();
        env->CallVoidMethod(motionProcessorObject, MotionProcessor_calibrate);
    }
    
    virtual Vec2 getDirectionVector() override
    {
        JNIEnv *env = JniHelper::getEnv();
        jfloatArray arr = env->NewFloatArray(2);
        env->CallVoidMethod(motionProcessorObject, MotionProcessor_getDirectionVector, arr);
        
        jfloat* elems = env->GetFloatArrayElements(arr, nullptr);
        Vec2 result(elems);
        env->ReleaseFloatArrayElements(arr, elems, JNI_ABORT);
        env->DeleteLocalRef(arr);
        
        return result;
    }
    
protected:
    MotionProcessorAndroid()
    {
        JNIEnv *env = JniHelper::getEnv();
        jclass MotionProcessorClass = env->FindClass("joaobapt/MotionProcessor");
        
        if (!methodsInited)
        {
            MotionProcessor_ctor = env->GetMethodID(MotionProcessorClass, "<init>", "()V");
            MotionProcessor_calibrate = env->GetMethodID(MotionProcessorClass, "calibrate", "()V");
            MotionProcessor_getDirectionVector = env->GetMethodID(MotionProcessorClass, "getDirectionVector", "([F)V");
            MotionProcessor_dispose = env->GetMethodID(MotionProcessorClass, "dispose", "()V");
            methodsInited = true;
        }
        
        jobject mprocessor = env->NewObject(MotionProcessorClass, MotionProcessor_ctor);
        motionProcessorObject = env->NewGlobalRef(mprocessor);
        env->DeleteLocalRef(mprocessor);
    }
    
    friend MotionProcessor *createMotionProcessor();
};

MotionProcessor *createMotionProcessor()
{
    return new MotionProcessorAndroid();
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_WINRT

using namespace Windows::Foundation;
using namespace Windows::Devices::Sensors;
using namespace Windows::Graphics::Display;

using OrHandler = TypedEventHandler<OrientationSensor^, OrientationSensorReadingChangedEventArgs^>;
using DiHandler = TypedEventHandler<DisplayInformation^, Platform::Object^>;

class MotionProcessorWin10 : public MotionProcessor
{
	OrientationSensor^ sensor;
	EventRegistrationToken orientationReadingHandlerToken, orientationChangedHandlerToken;

	Quaternion currentQuaternion, calibratedQuaternion;
	Vec2 directionVector;

public:
	virtual ~MotionProcessorWin10()
	{
		sensor->ReadingChanged -= orientationReadingHandlerToken;
		sensor = nullptr;

		DisplayInformation::GetForCurrentView()->OrientationChanged -= orientationChangedHandlerToken;
	}

	virtual void calibrate() override
	{
		calibratedQuaternion = currentQuaternion;
		calibratedQuaternion.conjugate();
		directionVector.set(0, 0);
	}

	virtual Vec2 getDirectionVector() override
	{
		return directionVector;
	}

protected:
	MotionProcessorWin10()
	{
		sensor = OrientationSensor::GetDefaultForRelativeReadings();
		sensor->ReadingTransform = DisplayOrientations::Landscape;

		uint32_t reportInterval = 17;
		if (reportInterval < sensor->MinimumReportInterval)
			reportInterval = sensor->MinimumReportInterval;
		sensor->ReportInterval = reportInterval;

		orientationReadingHandlerToken =
			sensor->ReadingChanged += ref new OrHandler(
				[=](OrientationSensor^ sensor, OrientationSensorReadingChangedEventArgs^ args)
		{
			auto squat = args->Reading->Quaternion;
			currentQuaternion.set(squat->X, squat->Y, squat->Z, squat->W);

			Quaternion quat = currentQuaternion * calibratedQuaternion;

			float tanpitch = .625 * (2 * quat.w*quat.y - 2 * quat.x*quat.z) / (1 - 2 * quat.y*quat.y - 2 * quat.z*quat.z);
			float tanroll = .625 * (2 * quat.w*quat.x - 2 * quat.y*quat.z) / (1 - 2 * quat.x*quat.x - 2 * quat.z*quat.z);

			directionVector = directionVector*(1 - K) + Vec2(tanroll, tanpitch)*K;
		});

		orientationChangedHandlerToken =
			DisplayInformation::GetForCurrentView()->OrientationChanged += ref new DiHandler(
				[=](DisplayInformation^ info, Platform::Object^ args)
		{
			CCLOG("Device being called!");
			if (sensor != nullptr)
				sensor->ReadingTransform = info->CurrentOrientation;
		});

		sensor->ReadingTransform = DisplayInformation::GetForCurrentView()->CurrentOrientation;
	}

	friend MotionProcessor *createMotionProcessor();
};

MotionProcessor *createMotionProcessor()
{
	return new MotionProcessorWin10();
}

#endif
