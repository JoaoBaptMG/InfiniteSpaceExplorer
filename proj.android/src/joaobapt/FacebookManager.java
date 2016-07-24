//
//  PictureDownloader.java
//  SpaceExplorer
//
//  Created by João Baptista on 20/09/15.
//
//

package joaobapt;

import java.lang.*;
import java.util.*;
import java.nio.ByteBuffer;
import android.os.*;
import com.facebook.*;
import com.facebook.login.*;
import org.cocos2dx.cpp.AppActivity;
import org.json.*;

class DirectorTracker extends AccessTokenTracker
{
    @Override
    protected native void onCurrentAccessTokenChanged(AccessToken old, AccessToken current);
}

class RedirectToNativeCallback implements GraphRequest.Callback
{
    public ByteBuffer nativeCallback;
    
    public RedirectToNativeCallback(ByteBuffer nativeCallback)
    {
        this.nativeCallback = nativeCallback;
    }
    
    native ByteBuffer nativeEmptyValue();
    native ByteBuffer nativeEmptyValueVector();
    native ByteBuffer nativeEmptyValueMap();
    native ByteBuffer nativeValueBoolean(boolean val);
    native ByteBuffer nativeValueInt(int val);
    native ByteBuffer nativeValueFloat(float val);
    native ByteBuffer nativeValueDouble(double val);
    native ByteBuffer nativeValueString(String val);
    
    native void nativeValueVectorAdd(ByteBuffer vec, ByteBuffer val);
    native void nativeValueMapAdd(ByteBuffer map, String key, ByteBuffer val);
    
    native void doCallback(ByteBuffer value, String str);
    
    public ByteBuffer nativeValue(Object obj)
    {
        try
        {
            if (obj instanceof Boolean)
                return nativeValueBoolean(((Boolean)obj).booleanValue());
            else if (obj instanceof Integer)
                return nativeValueInt(((Integer)obj).intValue());
            else if (obj instanceof Long)
                return nativeValueInt(((Long)obj).intValue());
            else if (obj instanceof Float)
                return nativeValueFloat(((Float)obj).floatValue());
            else if (obj instanceof Double)
                return nativeValueDouble(((Double)obj).doubleValue());
            else if (obj instanceof String)
                return nativeValueString((String)obj);
            else if (obj instanceof JSONArray)
            {
                ByteBuffer vec = nativeEmptyValueVector();
                for (int i = 0; i < ((JSONArray)obj).length(); i++)
                    nativeValueVectorAdd(vec, nativeValue(((JSONArray)obj).get(i)));
                return vec;
            }
            else if (obj instanceof JSONObject)
            {
                ByteBuffer map = nativeEmptyValueMap();
                
                Iterator<String> keys = ((JSONObject)obj).keys();
                while (keys.hasNext())
                {
                    String key = keys.next();
                    nativeValueMapAdd(map, key, nativeValue(((JSONObject)obj).get(key)));
                }
                
                return map;
            }
            else return nativeEmptyValue();
        }
        catch (JSONException exc)
        {
            return nativeEmptyValue();
        }
    }
    
    @Override
    public void onCompleted(GraphResponse response)
    {
        if (response.getError() == null)
        {
            try
            {
                ByteBuffer value = null;
                if (response.getJSONArray() != null)
                    value = nativeValue(response.getJSONArray());
                else if (response.getJSONObject().has(GraphResponse.NON_JSON_RESPONSE_PROPERTY))
                    value = nativeValue(response.getJSONObject().get(GraphResponse.NON_JSON_RESPONSE_PROPERTY));
                else value = nativeValue(response.getJSONObject());
                
                doCallback(value, "");
            }
            catch (JSONException exc)
            {
                doCallback(nativeEmptyValue(), exc.getMessage());
            }
        }
        else
        {
            doCallback(nativeEmptyValue(), response.getError().getErrorMessage());
        }
    }
}

public class FacebookManager implements FacebookCallback <LoginResult>
{
    final long UNKNOWN = 0, ERROR = 1, DECLINED = 2, ACCEPTED = 3;
    
    DirectorTracker tracker;
    long readState, publishState;
    
    ByteBuffer lastCallback;
    boolean wasLastCallbackPublish;
    CallbackManager callbackManager;
    
    public FacebookManager(CallbackManager callbackManager)
    {
        LoginManager.getInstance().registerCallback(callbackManager, this);
        this.callbackManager = callbackManager;
        
        tracker = new DirectorTracker();
        tracker.startTracking();
        
        readState = publishState = UNKNOWN;
        
        lastCallback = null;
    }
    
    public CallbackManager getCallbackManager()
    {
        return callbackManager;
    }
    
    public void cleanup()
    {
        tracker.stopTracking();
    }
    
    public void requestReadPermissions(ByteBuffer callback, boolean rerequest)
    {
        if (!rerequest && readState == DECLINED)
            doLoginCallback(callback, DECLINED, "");
        
        if (AccessToken.getCurrentAccessToken() != null)
        {
            Set<String> perms = AccessToken.getCurrentAccessToken().getPermissions();
            if (perms.contains("public_profile") && perms.contains("user_friends"))
            {
                readState = ACCEPTED;
                doLoginCallback(callback, ACCEPTED, "");
            }
        }
        
        lastCallback = callback;
        wasLastCallbackPublish = false;
        LoginManager.getInstance().logInWithReadPermissions(AppActivity.getActivity(),
                                                            Arrays.asList("public_profile", "user_friends"));
    }
    
    public void requestPublishPermissions(ByteBuffer callback, boolean rerequest)
    {
        if (!rerequest && publishState == DECLINED)
            doLoginCallback(callback, DECLINED, "");
        
        if (AccessToken.getCurrentAccessToken() != null)
        {
            Set<String> perms = AccessToken.getCurrentAccessToken().getPermissions();
            if (perms.contains("publish_actions"))
            {
                publishState = ACCEPTED;
                doLoginCallback(callback, ACCEPTED, "");
            }
        }
        
        lastCallback = callback;
        wasLastCallbackPublish = true;
        LoginManager.getInstance().logInWithPublishPermissions(AppActivity.getActivity(),
                                                               Arrays.asList("publish_actions"));
    }
    
    public native void doLoginCallback(ByteBuffer callback, long state, String errorMessage);
    
    public void logOut()
    {
        lastCallback = null;
        LoginManager.getInstance().logOut();
    }
    
    public boolean hasPermission(String permission)
    {
        AccessToken token = AccessToken.getCurrentAccessToken();
        
        if (token != null)
        {
            Set permissions = token.getPermissions();
            
            if (permissions != null)
                return permissions.contains(permission);
        }
        return false;
    }
    
    public void graphRequest(String path, Bundle params, long m, ByteBuffer callback)
    {
        HttpMethod method = m == 0 ? HttpMethod.GET : m == 1 ? HttpMethod.POST : HttpMethod.DELETE;
        
        GraphRequest request = new GraphRequest(AccessToken.getCurrentAccessToken(),
                                                path, params, method, new RedirectToNativeCallback(callback));
        request.executeAsync();
    }
    
    @Override
    public void onSuccess(LoginResult result)
    {
        if (lastCallback == null) return;
        
        if (wasLastCallbackPublish)
        {
            publishState = result.getRecentlyGrantedPermissions().contains("publish_actions") ? ACCEPTED : DECLINED;
            doLoginCallback(lastCallback, publishState, "");
        }
        else
        {
            readState = result.getRecentlyGrantedPermissions().contains("user_friends") ? ACCEPTED : DECLINED;
            doLoginCallback(lastCallback, readState, "");
        }
        lastCallback = null;
    }
    
    @Override
    public void onCancel()
    {
        if (lastCallback == null) return;
        
        doLoginCallback(lastCallback, UNKNOWN, "Login was cancelled.");
        lastCallback = null;
    }
    
    @Override
    public void onError(FacebookException ex)
    {
        if (lastCallback == null) return;
        
        doLoginCallback(lastCallback, ERROR, "Facebook error: " + ex.getLocalizedMessage());
        lastCallback = null;
    }
}