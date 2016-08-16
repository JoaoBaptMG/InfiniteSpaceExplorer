//
//  MotionProcessor.java
//  SpaceExplorer
//
//  Created by João Baptista on 26/09/15.
//
//

package joaobapt;

import java.lang.*;
import java.util.Arrays;
import android.hardware.*;
import android.view.Surface;
import android.content.Context;
import android.util.Log;
import android.os.*;
import org.cocos2dx.cpp.AppActivity;

class MotionProcessor implements SensorEventListener
{
    final float quatK = 0.4f;
    
    SensorManager manager;
    Sensor[] currentSensors;
    boolean manualSensorFusion;
    
    float[] currentQuaternion, calibratedQuaternion;
    //Matrix currentMatrix, calibratedMatrix;
    float[] directionVector;
    
    public MotionProcessor()
    {
        manager = (SensorManager)AppActivity.getActivity().getSystemService(Context.SENSOR_SERVICE);
        
        Sensor rotationVectorSensor;
        rotationVectorSensor = manager.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
        if (rotationVectorSensor == null)
            rotationVectorSensor = manager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
        
        if (rotationVectorSensor != null)
        {
            manualSensorFusion = false;
            
            currentSensors = new Sensor[] { rotationVectorSensor };
        }
        else
        {
            manualSensorFusion = true;
            
            Sensor accelerometer = manager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            Sensor gyroscope = manager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            Sensor magnetometer = manager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
            
            currentSensors = new Sensor[] { accelerometer, gyroscope, magnetometer };
        }
        
        for (Sensor sensor : currentSensors)
        {
            if (sensor != null)
            {
                Log.d("MotionProcessor", "Sensor type: " + sensor.getType());
                manager.registerListener(this, sensor, SensorManager.SENSOR_DELAY_FASTEST);
            }
        }
        
        currentQuaternion = new float[] { 0.0f, 0.0f, 0.0f, 0.0f };
        calibratedQuaternion = new float[] { 0.0f, 0.0f, 0.0f, 0.0f };
        directionVector = new float[] { 0.0f, 0.0f };
    }
    
    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
        
    }
    
    @Override
    public /*synchronized*/ void onSensorChanged(SensorEvent event)
    {
        if (!manualSensorFusion)
        {
            currentQuaternion[0] = event.values[0];
            currentQuaternion[1] = event.values[1];
            currentQuaternion[2] = event.values[2];
            
            if (event.values.length > 3)
            {
                currentQuaternion[3] = event.values[3];
                if (currentQuaternion[3] < 0.0f)
                {
                    currentQuaternion[0] = -currentQuaternion[0];
                    currentQuaternion[1] = -currentQuaternion[1];
                    currentQuaternion[2] = -currentQuaternion[2];
                    currentQuaternion[3] = -currentQuaternion[3];
                }
            }
            else
                currentQuaternion[3] = (float)Math.sqrt(1.0f - currentQuaternion[0]*currentQuaternion[0]
                                                             - currentQuaternion[1]*currentQuaternion[1]
                                                             - currentQuaternion[2]*currentQuaternion[2]);
            
            //Log.d("MotionProcessor", "currentQuaternion = " + Arrays.toString(currentQuaternion));
            
            float[] quat = new float[4];
            multQuaternion(currentQuaternion, calibratedQuaternion, quat);
            
            Log.d("MotionProcessor", "quat = " + Arrays.toString(quat));
            
            float tanpitch = .625f * (2.0f*quat[3]*quat[1] - 2.0f*quat[0]*quat[2]) / (1.0f - 2.0f*quat[1]*quat[1] - 2.0f*quat[2]*quat[2]);
            float tanroll = .625f * (2.0f*quat[3]*quat[0] - 2.0f*quat[1]*quat[2]) / (1.0f - 2.0f*quat[0]*quat[0] - 2.0f*quat[2]*quat[2]);
            
            float dx = 0.0f, dy = 0.0f;
            
            /*switch (AppActivity.getDisplayRotation())
            {
                case 0: dx =  tanpitch; dy = -tanroll;  break;
                case 1: dx =  tanroll;  dy =  tanpitch; break;
                case 2: dx =  tanpitch; dy = -tanroll;  break;
                case 3: dx =  tanroll;  dy =  tanpitch; break;
            }*/

			dx = tanpitch; dy = -tanroll;
            
            directionVector[0] = directionVector[0]*(1.0f-quatK) + dx*quatK;
            directionVector[1] = directionVector[1]*(1.0f-quatK) + dy*quatK;
            
            //Log.d("MotionProcessor", "directionVector = " + Arrays.toString(directionVector));
        }
    }
    
    public /*synchronized*/ void calibrate()
    {
        calibratedQuaternion[0] = -currentQuaternion[0];
        calibratedQuaternion[1] = -currentQuaternion[1];
        calibratedQuaternion[2] = -currentQuaternion[2];
        calibratedQuaternion[3] = currentQuaternion[3];
        
        Log.d("MotionProcessor", "calibrating");
        
        directionVector[0] = directionVector[1] = 0.0f;
    }
    
    public /*synchronized*/ void getDirectionVector(float[] vec)
    {
        vec[0] = directionVector[0];
        vec[1] = directionVector[1];
    }
    
    public /*synchronized*/ void dispose()
    {
        manager.unregisterListener(this);
        manager = null;
        currentSensors = null;
    }
    
    private static void multQuaternion(float[] a, float[] b, float[] c)
    {
        if (a.length != 4 || b.length != 4 || c.length != 4) return;
        
        c[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
        c[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
        c[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
        c[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
    }
}