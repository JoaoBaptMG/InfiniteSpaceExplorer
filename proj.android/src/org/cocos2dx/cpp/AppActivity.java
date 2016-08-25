/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2014 Chukong Technologies Inc.
 
http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
package org.cocos2dx.cpp;

import org.cocos2dx.lib.Cocos2dxActivity;
import android.os.Bundle;
import android.util.Log;
import android.content.*;
import android.app.Activity;
import android.view.*;
import android.net.Uri;
import com.facebook.*;
import com.facebook.appevents.AppEventsLogger;
import joaobapt.FacebookManager;

public class AppActivity extends Cocos2dxActivity
{
    public static Activity activity;
    public static FacebookManager fbManager;
    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        activity = this;
        
        FacebookSdk.sdkInitialize(getApplicationContext());
        fbManager = new FacebookManager(CallbackManager.Factory.create());
    }
    
    public static int getDisplayRotation()
    {
        return ((WindowManager)activity.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay().getRotation();
    }
    
    public static Activity getActivity()
    {
        return activity;
    }
    
    public static FacebookManager getFacebookManager()
    {
        return fbManager;
    }
    
    public static boolean openURL(String url)
    {
        try
        {
            activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
            return true;
        }
        catch (ActivityNotFoundException exc)
        {
            return false;
        }
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        AppEventsLogger.activateApp(this);
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        AppEventsLogger.deactivateApp(this);
    }
    
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        fbManager.cleanup();
    }
    
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        if (fbManager.getCallbackManager().onActivityResult(requestCode, resultCode, data)) return;
		gpgOnActivityResult(this, requestCode, resultCode, data);
    }

	private static native void gpgOnActivityResult(Activity activity, int requestCode, int resultCode, Intent data);
}
