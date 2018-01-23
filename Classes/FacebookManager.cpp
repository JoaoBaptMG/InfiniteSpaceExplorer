//
//  FacebookManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 06/07/15.
//
//

#include "FacebookManager.h"
#include "Defaults.h"
#include "DownloadPicture.h"

using namespace cocos2d;

static std::string appID = "";

static long lastLoadedFirst = -1, lastLoadedLast = -1;
static std::string lastBeforeCursor, lastAfterCursor;

static ScoreManager::ScoreData cachedPlayerData(-1, "", 0, true);
static bool cachedPlayerDataDirty = true;

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif

#import <FBSDKCoreKit/FBSDKCoreKit.h>
#import <FBSDKLoginKit/FBSDKLoginKit.h>
#import <FBSDKShareKit/FBSDKShareKit.h>

static FacebookManager::PermissionState readState = FacebookManager::PermissionState::UNKNOWN;
static FacebookManager::PermissionState publishState = FacebookManager::PermissionState::UNKNOWN;

void FacebookManager::initialize()
{
    appID = [FBSDKSettings appID].UTF8String;
    
    [[NSNotificationCenter defaultCenter] addObserverForName:FBSDKAccessTokenDidChangeNotification object:nil queue:nil
                                                  usingBlock:^(NSNotification *notification)
     {
         bool loggedIn = [FBSDKAccessToken currentAccessToken] != nil;
         Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("FacebookUpdated", &loggedIn);
     }];
}

bool FacebookManager::isAccessTokenValid()
{
    return [FBSDKAccessToken currentAccessToken] != NULL;
}

std::string FacebookManager::getUserID()
{
    return [FBSDKAccessToken currentAccessToken].userID.UTF8String;
}

std::string FacebookManager::getUserName()
{
    return [FBSDKProfile currentProfile].name.UTF8String;
}

void FacebookManager::requestReadPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest)
{
    if (!rerequest && readState == PermissionState::DECLINED)
        callback(PermissionState::DECLINED, "");
    
    if ([[FBSDKAccessToken currentAccessToken] hasGranted:@"public_profile"] && [[FBSDKAccessToken currentAccessToken] hasGranted:@"user_friends"])
    {
        callback(readState = PermissionState::ACCEPTED, "");
        return;
    }
    
    FBSDKLoginManager *loginManager = [[FBSDKLoginManager alloc] init];
    
    [loginManager logInWithReadPermissions:@[@"public_profile",@"user_friends"] fromViewController:nil handler:^(FBSDKLoginManagerLoginResult *result, NSError *error) {
        if (error)
        {
            callback(PermissionState::ERROR, error.localizedDescription.UTF8String);
        }
        else if (result.isCancelled)
        {
            callback(PermissionState::UNKNOWN, "Login was cancelled.");
        }
        else
        {
            readState = [result.grantedPermissions containsObject:@"user_friends"] ? PermissionState::ACCEPTED : PermissionState::DECLINED;
            callback(readState, "");
        }
    }];
}

void FacebookManager::requestPublishPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest)
{
    if (!rerequest && publishState == PermissionState::DECLINED)
        callback(PermissionState::DECLINED, "");
    
    if ([[FBSDKAccessToken currentAccessToken] hasGranted:@"publish_actions"])
    {
        callback(publishState = PermissionState::ACCEPTED, "");
        return;
    }
    
    FBSDKLoginManager *loginManager = [[FBSDKLoginManager alloc] init];
    
    [loginManager logInWithPublishPermissions:@[@"publish_actions"] fromViewController:nil handler:^(FBSDKLoginManagerLoginResult *result, NSError *error) {
        if (error)
        {
            callback(PermissionState::ERROR, error.localizedDescription.UTF8String);
        }
        else if (result.isCancelled)
        {
            callback(PermissionState::UNKNOWN, "Login was cancelled.");
        }
        else
        {
            publishState = [result.grantedPermissions containsObject:@"publish_actions"] ? PermissionState::ACCEPTED : PermissionState::DECLINED;
            callback(publishState, "");
        }
    }];
}

void FacebookManager::logOut()
{
    FBSDKLoginManager *loginManager = [[FBSDKLoginManager alloc] init];
    [loginManager logOut];
}

bool FacebookManager::hasPermission(std::string permission)
{
    return [[FBSDKAccessToken currentAccessToken] hasGranted:[NSString stringWithCString:permission.c_str() encoding:NSUTF8StringEncoding]];
}

inline void makeValueFromID(Value &value, id obj)
{
    if ([obj isKindOfClass:[NSNumber class]])
    {
        switch ([obj objCType][0])
        {
            case 'c': case 'C': value = (unsigned char)[obj charValue]; break;
            case 'i': case 'I': case 's': case 'S': case 'l': case 'L': case 'q': case 'Q':
                value = [obj intValue]; break;
            case 'f': value = [obj floatValue]; break;
            case 'd': value = [obj doubleValue]; break;
        }
    }
    else if ([obj isKindOfClass:[NSString class]])
        value = [obj UTF8String];
    else if ([obj isKindOfClass:[NSArray class]])
    {
        __block ValueVector vec;
        vec.reserve([obj count]);
        [obj enumerateObjectsUsingBlock:^ (id val, NSUInteger indx, BOOL *stop)
         {
			 vec.push_back(Value());
             makeValueFromID(vec.back(), val);
         }];
        value = std::move(vec);
    }
    else if ([obj isKindOfClass:[NSDictionary class]])
    {
        __block ValueMap map;
        [obj enumerateKeysAndObjectsUsingBlock:^ (id key, id val, BOOL *stop)
         {
             if (![key isKindOfClass:[NSString class]]) return;
             std::string keyStr = [key UTF8String];
             auto it = map.emplace(keyStr, Value()).first;
             makeValueFromID(it->second, val);
         }];
        value = std::move(map);
    }
}

void FacebookManager::graphRequest(std::string path, const std::unordered_map<std::string, std::string> &parameters, HTTPMethod method,
                                   std::function<void(Value&&, std::string)> callback)
{
    NSMutableDictionary *dict = [NSMutableDictionary dictionary];
    for (auto pair : parameters)
        [dict setValue:[NSString stringWithCString:pair.second.c_str() encoding:NSUTF8StringEncoding]
                forKey:[NSString stringWithCString:pair.first.c_str() encoding:NSUTF8StringEncoding]];
    
    [[[FBSDKGraphRequest alloc]
      initWithGraphPath:[NSString stringWithCString:path.c_str() encoding:NSUTF8StringEncoding]
      parameters:dict HTTPMethod:@[@"GET", @"POST", @"DELETE"][(std::size_t)method]]
     startWithCompletionHandler:^ (FBSDKGraphRequestConnection *connection, id result, NSError *error)
     {
         if (!error)
         {
             Value currentMap;
             makeValueFromID(currentMap, result);
             callback(std::move(currentMap), "");
         }
         else callback(Value(), error.localizedDescription.UTF8String);
     }];
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include <jni.h>
#include <string>
#include "platform/android/jni/JniHelper.h"

static jclass FacebookManagerClass, AccessTokenClass, AppActivity, Profile, BundleClass;
static jmethodID AppActivity_getFacebookManager;
static jmethodID FacebookManager_requestReadPermissions, FacebookManager_requestPublishPermissions;
static jmethodID FacebookManager_logOut, FacebookManager_hasPermission, FacebookManager_graphRequest;
static jmethodID AccessToken_getCurrentAccessToken, AccessToken_getUserId;
static jmethodID Profile_getCurrentProfile, Profile_getName;
static jmethodID Bundle_ctor, Bundle_putString;

void FacebookManager::initialize()
{
    JNIEnv *env = JniHelper::getEnv();
    
    AppActivity = (jclass)env->NewGlobalRef(env->FindClass("org/cocos2dx/cpp/AppActivity"));
    AppActivity_getFacebookManager = env->GetStaticMethodID(AppActivity, "getFacebookManager", "()Ljoaobapt/FacebookManager;");
    
    FacebookManagerClass = (jclass)env->NewGlobalRef(env->FindClass("joaobapt/FacebookManager"));
    FacebookManager_requestReadPermissions = env->GetMethodID(FacebookManagerClass, "requestReadPermissions", "(Ljava/nio/ByteBuffer;Z)V");
    FacebookManager_requestPublishPermissions = env->GetMethodID(FacebookManagerClass, "requestPublishPermissions", "(Ljava/nio/ByteBuffer;Z)V");
    FacebookManager_logOut = env->GetMethodID(FacebookManagerClass, "logOut", "()V");
    FacebookManager_hasPermission = env->GetMethodID(FacebookManagerClass, "hasPermission", "(Ljava/lang/String;)Z");
    FacebookManager_graphRequest = env->GetMethodID(FacebookManagerClass, "graphRequest", "(Ljava/lang/String;Landroid/os/Bundle;JLjava/nio/ByteBuffer;)V");
    
    AccessTokenClass = (jclass)env->NewGlobalRef(env->FindClass("com/facebook/AccessToken"));
    AccessToken_getCurrentAccessToken = env->GetStaticMethodID(AccessTokenClass, "getCurrentAccessToken", "()Lcom/facebook/AccessToken;");
    AccessToken_getUserId = env->GetMethodID(AccessTokenClass, "getUserId", "()Ljava/lang/String;");
    
    Profile = (jclass)env->NewGlobalRef(env->FindClass("com/facebook/Profile"));
    Profile_getCurrentProfile = env->GetStaticMethodID(Profile, "getCurrentProfile", "()Lcom/facebook/Profile;");
    Profile_getName = env->GetMethodID(Profile, "getName", "()Ljava/lang/String;");
    
    BundleClass = (jclass)env->NewGlobalRef(env->FindClass("android/os/Bundle"));
    Bundle_ctor = env->GetMethodID(BundleClass, "<init>", "()V");
    Bundle_putString = env->GetMethodID(BundleClass, "putString", "(Ljava/lang/String;Ljava/lang/String;)V");
    
    JniMethodInfo getAppId;
    if (!JniHelper::getStaticMethodInfo(getAppId, "com/facebook/FacebookSdk", "getApplicationId", "()Ljava/lang/String;")) return;
    
    jstring appIDStr = (jstring)getAppId.env->CallStaticObjectMethod(getAppId.classID, getAppId.methodID);
    appID = JniHelper::jstring2string(appIDStr);
    
    getAppId.env->DeleteLocalRef(appIDStr);
}

bool FacebookManager::isAccessTokenValid()
{
    JNIEnv *env = JniHelper::getEnv();
    
    jobject profile = env->CallStaticObjectMethod(AccessTokenClass, AccessToken_getCurrentAccessToken);
    bool result = profile != nullptr;
    
    if (result) env->DeleteLocalRef(profile);
    
    return result;
}

std::string FacebookManager::getUserID()
{
    JNIEnv *env = JniHelper::getEnv();
    
    jobject profile = env->CallStaticObjectMethod(AccessTokenClass, AccessToken_getCurrentAccessToken);
    if (profile == nullptr) return "";

    jstring id = (jstring)env->CallObjectMethod(profile, AccessToken_getUserId);
    std::string result = JniHelper::jstring2string(id);
    
    env->DeleteLocalRef(id);
    env->DeleteLocalRef(profile);
    
    return result;
}
                             
std::string FacebookManager::getUserName()
{
    JNIEnv *env = JniHelper::getEnv();
        
    jobject profile = env->CallStaticObjectMethod(Profile, Profile_getCurrentProfile);
    if (profile == nullptr) return "";
        
    jstring name = (jstring)env->CallObjectMethod(profile, Profile_getName);
    std::string result = JniHelper::jstring2string(name);
    
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(profile);
        
    return result;
}

void FacebookManager::requestReadPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest)
{
    JNIEnv *env = JniHelper::getEnv();
    jobject obj = env->NewDirectByteBuffer((void*)(new std::function<void(FacebookManager::PermissionState, std::string)>(callback)),
                                           sizeof(std::function<void(FacebookManager::PermissionState, std::string)>));
    
    jobject manager = env->CallStaticObjectMethod(AppActivity, AppActivity_getFacebookManager);
    env->CallVoidMethod(manager, FacebookManager_requestReadPermissions, obj, rerequest);
    
    env->DeleteLocalRef(manager);
    env->DeleteLocalRef(obj);
}

void FacebookManager::requestPublishPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest)
{
    JNIEnv *env = JniHelper::getEnv();
    jobject obj = env->NewDirectByteBuffer((void*)(new std::function<void(FacebookManager::PermissionState, std::string)>(callback)),
                                           sizeof(std::function<void(FacebookManager::PermissionState, std::string)>));
    
    jobject manager = env->CallStaticObjectMethod(AppActivity, AppActivity_getFacebookManager);
    env->CallVoidMethod(manager, FacebookManager_requestPublishPermissions, obj, rerequest);
    
    env->DeleteLocalRef(manager);
    env->DeleteLocalRef(obj);
}

void FacebookManager::logOut()
{
    JNIEnv *env = JniHelper::getEnv();
    
    jobject manager = env->CallStaticObjectMethod(AppActivity, AppActivity_getFacebookManager);
    env->CallVoidMethod(manager, FacebookManager_logOut);
    
    env->DeleteLocalRef(manager);
}

bool FacebookManager::hasPermission(std::string permission)
{
    JNIEnv *env = JniHelper::getEnv();
    
    jstring str = env->NewStringUTF(permission.c_str());
    
    jobject manager = env->CallStaticObjectMethod(AppActivity, AppActivity_getFacebookManager);
    jboolean result = env->CallBooleanMethod(manager, FacebookManager_hasPermission, str);
    
    env->DeleteLocalRef(str);
    env->DeleteLocalRef(manager);
    
    return result;
}

void FacebookManager::graphRequest(std::string path, const std::unordered_map<std::string, std::string> &parameters,
                                   HTTPMethod method, std::function<void(Value&&, std::string)> callback)
{
    JNIEnv *env = JniHelper::getEnv();
    
    jobject bundle = env->NewObject(BundleClass, Bundle_ctor);
    
    for (const auto &val : parameters)
    {
        jstring key = env->NewStringUTF(val.first.c_str());
        jstring value = env->NewStringUTF(val.second.c_str());
        
        env->CallVoidMethod(bundle, Bundle_putString, key, value);
        
        env->DeleteLocalRef(value);
        env->DeleteLocalRef(key);
    }
    
    jstring pathStr = env->NewStringUTF(path.c_str());
    jobject obj = env->NewDirectByteBuffer((void*)(new std::function<void(Value&&, std::string)>(callback)), sizeof(std::function<void(Value&&, std::string)>));
    
    jobject manager = env->CallStaticObjectMethod(AppActivity, AppActivity_getFacebookManager);
    env->CallVoidMethod(manager, FacebookManager_graphRequest, pathStr, bundle, (jlong)method, obj);
    
    env->DeleteLocalRef(manager);
    env->DeleteLocalRef(bundle);
    env->DeleteLocalRef(pathStr);
    env->DeleteLocalRef(obj);
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
#define APP_ID "1616893238593550"

using namespace cocos2d;
using namespace Windows::Foundation::Collections;
using namespace Windows::Data::Json;
using namespace concurrency;
using namespace winsdkfb;
using namespace winsdkfb::Graph;

using WStringVector = Platform::Collections::Vector<Platform::String^>;

static FacebookManager::PermissionState readState = FacebookManager::PermissionState::UNKNOWN;
static FacebookManager::PermissionState publishState = FacebookManager::PermissionState::UNKNOWN;

void FacebookManager::initialize()
{
	FBSession^ session = FBSession::ActiveSession;
	session->FBAppId = APP_ID;
	session->WinAppId = "TODO-FILL-THIS";

	appID = APP_ID;
}

bool FacebookManager::isAccessTokenValid()
{
	return FBSession::ActiveSession->LoggedIn;
}

std::string FacebookManager::getUserID()
{
	if (isAccessTokenValid() && FBSession::ActiveSession->User)
		return PlatformStringToString(FBSession::ActiveSession->User->Id);
	return "";
}

std::string FacebookManager::getUserName()
{
	if (isAccessTokenValid() && FBSession::ActiveSession->User)
		return PlatformStringToString(FBSession::ActiveSession->User->Name);
	return "";
}

void FacebookManager::requestReadPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest)
{
	if (!rerequest && readState == FacebookManager::PermissionState::DECLINED)
		callback(FacebookManager::PermissionState::DECLINED, "");

	if (FacebookManager::hasPermission("public_profile") && FacebookManager::hasPermission("user_friends"))
		callback(readState = FacebookManager::PermissionState::ACCEPTED, "");

	auto list = ref new WStringVector();
	list->Append("public_profile");
	list->Append("user_friends");
	auto permissions = ref new FBPermissions(list->GetView());

	create_task(FBSession::ActiveSession->LoginAsync(permissions)).then([=](FBResult^ result)
	{
		using namespace FacebookManager;
		if (result->Succeeded)
		{
			readState = hasPermission("user_friends") ? PermissionState::ACCEPTED : PermissionState::DECLINED;
			callback(readState, "");
		}
		else callback(PermissionState::ERROR, PlatformStringToString(result->ErrorInfo->Message));
	});
}

void FacebookManager::requestPublishPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest)
{
	if (!rerequest && publishState == FacebookManager::PermissionState::DECLINED)
		callback(FacebookManager::PermissionState::DECLINED, "");

	if (FacebookManager::hasPermission("publish_actions"))
		callback(publishState = FacebookManager::PermissionState::ACCEPTED, "");

	auto list = ref new WStringVector();
	list->Append("publish_actions");
	auto permissions = ref new FBPermissions(list->GetView());

	create_task(FBSession::ActiveSession->LoginAsync(permissions)).then([=](FBResult^ result)
	{
		using namespace FacebookManager;
		if (result->Succeeded)
		{
			publishState = hasPermission("publish_actions") ? PermissionState::ACCEPTED : PermissionState::DECLINED;
			callback(publishState, "");
		}
		else callback(PermissionState::ERROR, PlatformStringToString(result->ErrorInfo->Message));
	});
}

void FacebookManager::logOut()
{
	FBSession::ActiveSession->LogoutAsync();
}

bool FacebookManager::hasPermission(std::string permission)
{
	auto accessTokenData = FBSession::ActiveSession->AccessTokenData;
	if (!accessTokenData) return false;

	return accessTokenData->GrantedPermissions->Values->IndexOf(PlatformStringFromString(permission), nullptr);
}

inline void makeValueFromJsonValue(Value& value, IJsonValue^ jval)
{
	switch (jval->ValueType)
	{
		case JsonValueType::Boolean: value = Value(jval->GetBoolean()); break;
		case JsonValueType::Number: value = Value(jval->GetNumber()); break;
		case JsonValueType::String: value = Value(PlatformStringToString(jval->GetString())); break;
		case JsonValueType::Array:
		{
			ValueVector vector;
			for each (auto jelm in jval->GetArray())
			{
				vector.push_back(Value());
				makeValueFromJsonValue(vector.back(), jelm);
			}
			value = Value(std::move(vector));
		} break;
		case JsonValueType::Object:
		{
			ValueMap map;
			for each (auto jpair in jval->GetObject())
			{
				auto iter = map.emplace(PlatformStringToString(jpair->Key), Value()).first;
				makeValueFromJsonValue(iter->second, jpair->Value);
			}
			value = Value(std::move(map));
		} break;
		case JsonValueType::Null: value = Value(); break;
	}
}

void FacebookManager::graphRequest(std::string path, const std::unordered_map<std::string, std::string> &parameters,
	HTTPMethod method, std::function<void(Value&&, std::string)> callback)
{
	auto params = ref new PropertySet();
	for (auto pair : parameters)
		params->Insert(PlatformStringFromString(pair.first), PlatformStringFromString(pair.second));

	auto factory = ref new FBJsonClassFactory([](Platform::String^ json) { return JsonValue::Parse(json); });
	auto sval = ref new FBSingleValue(PlatformStringFromString(path), params, factory);

	auto task = method == HTTPMethod::GET ? sval->GetAsync() :
		method == HTTPMethod::POST ? sval->PostAsync() :
		sval->DeleteAsync();

	create_task(task).then([=](FBResult^ result)
	{
		if (result->Succeeded)
		{
			Value value;
			makeValueFromJsonValue(value, static_cast<IJsonValue^>(result->Object));
			callback(std::move(value), "");
		}
		else callback(Value(), PlatformStringToString(result->ErrorInfo->Message));
	});
}

#endif

void FacebookManager::loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler)
{
    if (cachedPlayerDataDirty)
        loadHighscoresOnRange(ScoreManager::SocialConstraint::FRIENDS, ScoreManager::TimeConstraint::ALL, 1, 5000,
                              [=] (long, std::vector<ScoreManager::ScoreData>&&, std::string)
                              {
                                  handler(cachedPlayerData);
                              });
    else handler(cachedPlayerData);
}

void FacebookManager::loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
                                            long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler, bool loadPhotos)
{
    CCASSERT(socialConstraint == ScoreManager::SocialConstraint::FRIENDS, "Only friends social constraint is supported on Facebook");
    CCASSERT(timeConstraint == ScoreManager::TimeConstraint::ALL, "Only all time constraint is supported on Facebook");
    
    if (hasPermission("user_friends"))
    {
        std::string size = ulongToString(48 * Director::getInstance()->getContentScaleFactor());
        std::unordered_map<std::string, std::string> params;
        params.emplace("fields", "user{name,picture.width(" + size + ").height(" + size + ")},score");
        params.emplace("limit", ulongToString(last-first+1));
        
        if (last == lastLoadedFirst - 1) params.emplace("before", lastBeforeCursor);
        else if (first == lastLoadedLast + 1) params.emplace("after", lastAfterCursor);
        else params.emplace("offset", ulongToString(first-1));
        
        graphRequest(appID + "/scores", params, HTTPMethod::GET, [=] (Value&& value, std::string error)
        {
            if (value.getType() == Value::Type::MAP)
            {
                auto data = value.asValueMap().find("data");
                
                if (data != value.asValueMap().end())
                {
                    std::string userID = getUserID();
                    
                    std::vector<ScoreManager::ScoreData> scores;
                    long current = first;
                    
                    for (const auto &val : data->second.asValueVector())
                    {
                        const auto &map = val.asValueMap();
                        ScoreManager::ScoreData score;
                        
                        score.index = current++;
                        score.score = map.at("score").asInt();
                        
                        const auto &userData = map.at("user").asValueMap();
                        score.name = userData.at("name").asString();
                        score.isPlayer = userData.at("id").asString() == userID;
                        std::string textureKey = score.textureKey = "Picture" + userData.at("id").asString();
                        
                        const auto &userPicture = userData.at("picture").asValueMap().at("data").asValueMap();
                        if (loadPhotos && !userPicture.at("is_silhouette").asBool() && Director::getInstance()->getTextureCache()->getTextureForKey(score.textureKey) == nullptr)
                        {
                            downloadPicture(userPicture.at("url").asString(), textureKey, [=] (Texture2D* texture)
                            {
                                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("TextureArrived." + textureKey, &texture);
                            });
                        }
                        
                        scores.push_back(std::move(score));
                        
                        if (score.isPlayer)
                        {
                            cachedPlayerDataDirty = false;
                            cachedPlayerData = score;
                        }
                    }
                    
                    handler(first, std::move(scores), "");
                }
                else
                {
                    std::string error = value.asValueMap().at("error").asValueMap().at("message").asString();
                    handler(-1, std::vector<ScoreManager::ScoreData>(), error);
                }
                
                auto paging = value.asValueMap().find("paging");
                if (paging != value.asValueMap().end())
                {
                    auto cursors = paging->second.asValueMap().find("cursors");
                    if (cursors != paging->second.asValueMap().end())
                    {
                        lastLoadedFirst = first;
                        lastLoadedLast = last;
                        
                        auto it = cursors->second.asValueMap().find("before");
                        if (it != cursors->second.asValueMap().end())
                            lastBeforeCursor = it->second.asString();
                        
                        it = cursors->second.asValueMap().find("after");
                        if (it != cursors->second.asValueMap().end())
                            lastAfterCursor = it->second.asString();
                    }
                }
            }
            else handler(-1, std::vector<ScoreManager::ScoreData>(), error);
        });
    }
    else handler(-1, std::vector<ScoreManager::ScoreData>(), "You have not allowed the application to fetch your friends' scores.");
}

void FacebookManager::reportScore(int64_t score)
{
    if (hasPermission("public_profile") && hasPermission("publish_actions"))
    {
        graphRequest("me/scores", { { "fields", "score" } }, HTTPMethod::GET, [=] (Value&& map, std::string)
        {
            if (map.getType() == Value::Type::MAP)
            {
                int64_t curScore = map.asValueMap().at("data").asValueVector().at(0).asValueMap().at("score").asInt();
                
                if (curScore < score)
                    graphRequest("me/scores", { { "score", std::to_string(score) } }, HTTPMethod::POST, [] (Value&&, std::string) {});
            }
        });
    }
}
