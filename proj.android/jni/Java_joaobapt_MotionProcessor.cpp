//
//  Java_joaobapt_MotionProcessor.cpp
//
//
//  Created by João Baptista on 16/08/16.
//
//

#include <jni.h>
#include "cocos2d.h"

using namespace cocos2d;

extern "C" JNIEXPORT jobject JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionBufferCreate(JNIEnv* env, jobject thiz, jboolean usesGyroscope);
extern "C" JNIEXPORT jfloatArray JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionResolve(JNIEnv* env, jobject thiz, jobject buffer);
extern "C" JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionBufferFree(JNIEnv* env, jobject thiz, jobject buffer);
extern "C" JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionFeedAccelerometerData(JNIEnv* env, jobject thiz, jfloatArray accData, jobject buffer);
extern "C" JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionFeedMagnetometerData(JNIEnv* env, jobject thiz, jfloatArray magData, jobject buffer);
extern "C" JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionFeedGyroscopeData(JNIEnv* env, jobject thiz, jfloatArray gyroData, jlong timestamp, jobject buffer);

struct manualSensorFusionData
{
	Vec3 lastAccelerometerData{ 0, 0, 0 }, lastMagnetometerData{ 0, 0, 0 };
	Quaternion accumulatedGyroscopeData{ 0, 0, 0, 0 };
	uint64_t timestamp{ 0 };
	bool usesGyroscope{ false };
};

Vec3 vec3FromArray(JNIEnv *env, jfloatArray arr)
{
	float buffer[3];
	env->GetFloatArrayRegion(arr, 0, 3, buffer);
	return Vec3(buffer);
}

constexpr float K = 0.5, L = 0.8;

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

Quaternion resolveManualSensorFusion(manualSensorFusionData *data)
{
	Quaternion result{ 0, 0, 0, 0 };

	if (data->usesGyroscope)
	{
		Quaternion accelMagRotation;
		getRotationQuaternion(data->lastAccelerometerData, data->lastMagnetometerData, &accelMagRotation);
		Quaternion::slerp(accelMagRotation, data->accumulatedGyroscopeData, L, &result);
		data->accumulatedGyroscopeData = result;
	}
	else getRotationQuaternion(data->lastAccelerometerData, data->lastMagnetometerData, &result);

	return result;
}

JNIEXPORT jobject JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionBufferCreate(JNIEnv* env, jobject thiz, jboolean usesGyroscope)
{
	auto buffer = new manualSensorFusionData();
	buffer->usesGyroscope = usesGyroscope;

    return env->NewDirectByteBuffer((void*)buffer, sizeof(manualSensorFusionData));
}

JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionFeedAccelerometerData(JNIEnv* env, jobject thiz, jfloatArray accData, jobject buffer)
{
	manualSensorFusionData *data = reinterpret_cast<manualSensorFusionData*>(env->GetDirectBufferAddress(buffer));

	if (data->usesGyroscope) data->lastAccelerometerData = vec3FromArray(env, accData);
	else data->lastAccelerometerData += K*(vec3FromArray(env, accData) - data->lastAccelerometerData);

	//env->DeleteLocalRef(buffer);
}

JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionFeedMagnetometerData(JNIEnv* env, jobject thiz, jfloatArray magData, jobject buffer)
{
	manualSensorFusionData *data = reinterpret_cast<manualSensorFusionData*>(env->GetDirectBufferAddress(buffer));

	if (data->usesGyroscope) data->lastMagnetometerData = vec3FromArray(env, magData);
	else data->lastMagnetometerData += K*(vec3FromArray(env, magData) - data->lastMagnetometerData);

	//env->DeleteLocalRef(buffer);
}

JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionFeedGyroscopeData(JNIEnv* env, jobject thiz, jfloatArray gyroData, jlong timestamp, jobject buffer)
{
	manualSensorFusionData *data = reinterpret_cast<manualSensorFusionData*>(env->GetDirectBufferAddress(buffer));

	if (!data->lastAccelerometerData.isZero() && !data->lastMagnetometerData.isZero())
	{
		if (data->timestamp != 0)
		{
			auto axes = vec3FromArray(env, gyroData);
			float length = axes.length();
			if (length > 0.000000001f)
			{
				axes.normalize();
				float angle = length * float(timestamp - data->timestamp) / 1000000000.0f;
				float cosAngle = cosf(angle), sinAngle = sinf(angle);
				axes.scale(sinAngle);

				data->accumulatedGyroscopeData = Quaternion(axes.x, axes.y, axes.z, cosAngle) * data->accumulatedGyroscopeData;
			}
		}
		data->timestamp = timestamp;
	}

	//env->DeleteLocalRef(buffer);
}

Quaternion resolveManualSensorFusion(Vec3 accData, Vec3 magData, Quaternion gyroData, manualSensorFusionData *sensorData);

JNIEXPORT jfloatArray JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionResolve(JNIEnv* env, jobject thiz, jobject buffer)
{
	manualSensorFusionData *data = reinterpret_cast<manualSensorFusionData*>(env->GetDirectBufferAddress(buffer));

	Quaternion quat = resolveManualSensorFusion(data);

	jfloatArray result = env->NewFloatArray(4);
	float res[4] = { quat.x, quat.y, quat.z, quat.w };
	env->SetFloatArrayRegion(result, 0, 4, res);

	//env->DeleteLocalRef(buffer);

	return result;
}

JNIEXPORT void JNICALL Java_joaobapt_MotionProcessor_manualSensorFusionBufferFree(JNIEnv* env, jobject thiz, jobject buffer)
{
	manualSensorFusionData *data = reinterpret_cast<manualSensorFusionData*>(env->GetDirectBufferAddress(buffer));
	delete data;

	//env->DeleteLocalRef(buffer);
}
