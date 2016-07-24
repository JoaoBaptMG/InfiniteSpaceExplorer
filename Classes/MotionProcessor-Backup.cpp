//
//  MotionProcessor.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 06/03/15.
// 
//

#if 0 // Prevent compilation

#include "MotionProcessor.h"

using namespace cocos2d;

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#import <CoreMotion/CoreMotion.h>
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include <android/sensor.h>
#include "platform/android/jni/JniHelper.h"
#include <cstring>
constexpr int LOOPER_ID = 12;

struct SensorData
{
    ALooper *looper;
    ASensorManager *sensorManager;
    ASensorEventQueue *queue;
    ASensorRef sensors[3];
    cocos2d::Vec3 lastAccelerometerData, lastMagnetometerData;
    cocos2d::Quaternion accumulatedGyroscopeData;
    bool manualSensorFusion, usesMagnetometer, usesGyroscope;
};

inline static bool getRotationQuaternion(Vec3 gravity, const Vec3 &geomagnetic, Quaternion *outQuat)
{
    Vec3 H, M;
    Vec3::cross(geomagnetic, gravity, &H);
    if (H.lengthSquared() < 0.01) return false;
    H.normalize();
    gravity.normalize();
    Vec3::cross(gravity, H, &M);
    
    Quaternion::createFromRotationMatrix(Mat4(H.x, H.y, H.z, 0, M.x, M.y, M.z, 0, gravity.x, gravity.y, gravity.z, 0, 0, 0, 0, 1), outQuat);
    return true;
}
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8

#endif

MotionProcessor::MotionProcessor() : calibratedQuaternion(0, 0, 0, 0), directionVector(0, 0)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    motionSource = (void*)[CMMotionManager new];
    [(CMMotionManager*)motionSource startDeviceMotionUpdatesUsingReferenceFrame:CMAttitudeReferenceFrameXArbitraryZVertical];
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    SensorData *data = new SensorData();
    
    data->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    data->sensorManager = ASensorManager_getInstance();
    
    data->manualSensorFusion = false;
    data->sensors[0] = ASensorManager_getDefaultSensor(data->sensorManager, 15); // Try to get the TYPE_GAME_ROTATION_VECTOR
    if (data->sensors[0] == nullptr) data->sensors[0] = ASensorManager_getDefaultSensor(data->sensorManager, 11); // TYPE_ROTATION_VECTOR
    if (data->sensors[0] == nullptr)
    {
        data->manualSensorFusion = true;
        data->sensors[0] = ASensorManager_getDefaultSensor(data->sensorManager, ASENSOR_TYPE_ACCELEROMETER);
        
        data->sensors[1] = ASensorManager_getDefaultSensor(data->sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
        data->usesMagnetometer = data->sensors[1] != nullptr;
        
        data->sensors[2] = ASensorManager_getDefaultSensor(data->sensorManager, ASENSOR_TYPE_GYROSCOPE);
        data->usesGyroscope = data->sensors[2] != nullptr;
    }
    
    data->lastAccelerometerData.set(0, 0, 0);
    data->lastMagnetometerData.set(0, 0, 0);
    data->accumulatedGyroscopeData.set(0, 0, 0, 0);
    
    data->queue = ASensorManager_createEventQueue(data->sensorManager, data->looper, LOOPER_ID, nullptr, nullptr);
    
    for (int i = 0; i < sizeof(data->sensors)/sizeof(data->sensors[0]); i++)
    {
        if (data->sensors[i] == nullptr) continue;
        ASensorEventQueue_enableSensor(data->queue, data->sensors[i]);
        ASensorEventQueue_setEventRate(data->queue, data->sensors[i], 16667);
    }
    
    motionSource = (void*)data;
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8
    
#endif
}

MotionProcessor::~MotionProcessor()
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    [(CMMotionManager*)motionSource stopDeviceMotionUpdates];
    [(CMMotionManager*)motionSource release];
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    SensorData *data = (SensorData*)motionSource;
    
    for (int i = 0; i < sizeof(data->sensors)/sizeof(data->sensors[0]); i++)
        if (data->sensors[i] != nullptr) ASensorEventQueue_disableSensor(data->queue, data->sensors[i]);
    
    ASensorManager_destroyEventQueue(data->sensorManager, data->queue);
    delete data;
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8
    
#endif
}

Quaternion MotionProcessor::getSourceQuaternion()
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    if (((CMMotionManager*)motionSource).deviceMotion == nil || ((CMMotionManager*)motionSource).deviceMotion.attitude == nil)
        return Quaternion(0, 0, 0, 0);
    else
    {
        CMQuaternion quat = ((CMMotionManager*)motionSource).deviceMotion.attitude.quaternion;
        Quaternion result(quat.x, quat.y, quat.z, quat.w);
        
        return result;
    }
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    constexpr float K = 0.2, L = 0.4;
    
    int events;
    int looperID = ALooper_pollAll(1, NULL, &events, NULL);
    
    if (looperID == ALOOPER_POLL_TIMEOUT || looperID == ALOOPER_POLL_ERROR)
        return Quaternion(0, 0, 0, 0);
    
    SensorData *data = (SensorData*)motionSource;
    
    if (!data->sensors[0]) return Quaternion(0, 0, 0, 0);
    Quaternion result(0, 0, 0, 0);
    
    /*while (looperID >= 0)
    {*/
        if (looperID == LOOPER_ID)
        {
            ASensorEvent event;
            while (ASensorEventQueue_getEvents(data->queue, &event, 1) > 0)
            {
                if (event.type == 15 || event.type == 11)
                {
                    log("Event data: %g %g %g %g", event.data[0], event.data[1], event.data[2], event.data[3]);
                    event.data[3] = sqrtf(1 - Vec3(event.data).lengthSquared());
                    result.set(event.data);
                }
                else if (event.type == ASENSOR_TYPE_ACCELEROMETER)
                {
                    if (data->usesGyroscope) data->lastAccelerometerData.set(event.data);
                    else data->lastAccelerometerData += K*(Vec3(event.data) - data->lastAccelerometerData);
                }
                else if (event.type == ASENSOR_TYPE_MAGNETIC_FIELD)
                {
                    if (data->usesGyroscope) data->lastMagnetometerData.set(event.data);
                    else data->lastMagnetometerData += K*(Vec3(event.data) - data->lastMagnetometerData);
                }
                else if (event.type == ASENSOR_TYPE_GYROSCOPE)
                {
                    if (data->lastAccelerometerData.isZero() || data->lastMagnetometerData.isZero()) continue;
                    static uint64_t timestamp = 0;
                    
                    if (timestamp != 0)
                    {
                        Vec3 axes(event.data);
                        float length = axes.length();
                        if (length > 0.000000001f)
                        {
                            axes.normalize();
                            float angle = length * float(event.timestamp - timestamp) / 1000000000.0f;
                            float cosAngle = cosf(angle), sinAngle = sinf(angle);
                            axes.scale(sinAngle);
                            
                            data->accumulatedGyroscopeData = Quaternion(axes.x, axes.y, axes.z, cosAngle) * data->accumulatedGyroscopeData;
                        }
                    }
                    timestamp = event.timestamp;
                }
                else CCLOG("Strange sensor: %d - data: %g %g %g", event.type, event.data[0], event.data[1], event.data[2]);
            }
        }
        /*looperID = ALooper_pollAll(1, NULL, &events, NULL);
    }*/
    
    if (data->manualSensorFusion && (data->usesMagnetometer || data->usesGyroscope))
    {
        if (!data->usesMagnetometer) result.set(data->accumulatedGyroscopeData);
        else if (!data->usesGyroscope) getRotationQuaternion(data->lastAccelerometerData, data->lastMagnetometerData, &result);
        else
        {
            Quaternion accelMagRotation;
            getRotationQuaternion(data->lastAccelerometerData, data->lastMagnetometerData, &accelMagRotation);
            Quaternion::slerp(accelMagRotation, data->accumulatedGyroscopeData, L, &result);
            data->accumulatedGyroscopeData = result;
        }
    }
    
    return result;
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8
    
#endif
}

void MotionProcessor::calibrate()
{
    calibratedQuaternion = getSourceQuaternion();
    calibratedQuaternion.conjugate();
    
    directionVector.set(0, 0);
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    SensorData *data = (SensorData*)motionSource;
    if (data->usesGyroscope) data->accumulatedGyroscopeData.set(0, 0, 0, 0);
#endif
}

constexpr float K = 0.4;
void MotionProcessor::update()
{
    auto quat = getSourceQuaternion();
    if (!quat.isZero())
    {
        quat = calibratedQuaternion * quat;
        
        float tanpitch = .625 * (2*quat.w*quat.y - 2*quat.x*quat.z) / (1 - 2*quat.y*quat.y - 2*quat.z*quat.z);
        float tanroll = .625 * (2*quat.w*quat.x - 2*quat.y*quat.z) / (1 - 2*quat.x*quat.x - 2*quat.z*quat.z);

        Vec2 values;
        
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        if ([UIApplication sharedApplication].statusBarOrientation == UIInterfaceOrientationLandscapeRight)
            values.set(tanroll, tanpitch);
        else
            values.set(-tanroll, -tanpitch);
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        JniMethodInfo t;
        if (JniHelper::getStaticMethodInfo(t, "org/cocos2dx/cpp/AppActivity", "getDisplayRotation", "()I"))
        {
            jint rotation = t.env->CallStaticIntMethod(t.classID, t.methodID);
            t.env->DeleteLocalRef(t.classID);
            
            switch (rotation)
            {
                case 0: values.set(tanpitch, -tanroll); break;
                case 1: values.set(tanroll, tanpitch); break;
                case 2: values.set(-tanpitch, tanroll); break;
                case 3: values.set(-tanroll, -tanpitch); break;
            }
        }
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8

#endif
        
        directionVector = directionVector*(1-K) + values*K;
    }
}

#endif