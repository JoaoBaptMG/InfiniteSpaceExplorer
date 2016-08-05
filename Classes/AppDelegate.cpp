#include "AppDelegate.h"
#include "MultiPurposeScene.h"
#include "Defaults.h"
#include "CustomGLPrograms.h"
#include "BlurFilter.h"
#include "ScoreManager.h"
#include "FacebookManager.h"
#include "SoundManager.h"
#include "audio/include/SimpleAudioEngine.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif

#import <UIKit/UIKit.h>
#import "GameCenterManager.h"
#include "GPGManager.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "renderer/CCTextureCache.h"
#include "GPGManager.h"
#endif

USING_NS_CC;

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate() 
{
}

// if you want a different context, modify the value of glContextAttrs
// it will affect all platforms
void AppDelegate::initGLContextAttrs()
{
    // set OpenGL context attributes: red,green,blue,alpha,depth,stencil
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8};

    GLView::setGLContextAttrs(glContextAttrs);
}

void AppDelegate::setDesignResolution(cocos2d::GLView *view)
{
    auto size = view->getFrameSize();
    
    float scaleFactor;
    
    CCLOG("Frame size: %g %g", size.width, size.height);
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    scaleFactor = view->getContentScaleFactor();
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) scaleFactor *= 2;
#else
    // Bultin scale factor computation for other platforms
    if (size.height <= 225) scaleFactor = 0.5;
    else if (size.height <= 300) scaleFactor = 0.75;
    else if (size.height <= 450) scaleFactor = 1;
    else if (size.height <= 600) scaleFactor = 1.5;
    else if (size.height <= 800) scaleFactor = 2;
    else if (size.height <= 1000) scaleFactor = 2.5;
    else scaleFactor = ceil(size.height/400);
#endif
    
    CCLOG("New scale factor: %g", scaleFactor);
    
    // set the design resolution
    view->setDesignResolutionSize(size.width/scaleFactor, size.height/scaleFactor, ResolutionPolicy::SHOW_ALL);
    
    // set the scale factor
    float resourceScaleFactor = ceilf(scaleFactor);
    if (resourceScaleFactor > 4.0f) resourceScaleFactor = 4.0f;
    
    Director::getInstance()->setContentScaleFactor(resourceScaleFactor);
    
    std::string path = "assets0x";
    path[6] += (char)resourceScaleFactor;
    FileUtils::getInstance()->setSearchPaths({ path });
}

void setUserDefaults()
{
    if (UserDefault::getInstance()->getIntegerForKey("MusicVolume", -1) == -1)
        UserDefault::getInstance()->setIntegerForKey("MusicVolume", 100);
    
    if (UserDefault::getInstance()->getIntegerForKey("SoundVolume", -1) == -1)
        UserDefault::getInstance()->setIntegerForKey("SoundVolume", 100);
    
    if (UserDefault::getInstance()->getIntegerForKey("TiltSensitivity", -1) == -1)
        UserDefault::getInstance()->setIntegerForKey("TiltSensitivity", 64);
    
    if (UserDefault::getInstance()->getIntegerForKey("AccumulatedScore", -1) == -1)
        UserDefault::getInstance()->setIntegerForKey("AccumulatedScore", 0);
}

// if you want to use the package manager to install more packages,  
// don't modify or remove this function
static int register_all_packages()
{
    return 0; //flag for packages manager
}

bool AppDelegate::applicationDidFinishLaunching() {
    // initialize director
    auto director = Director::getInstance();
    auto glview = director->getOpenGLView();
    if(!glview) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
        glview = GLViewImpl::createWithRect("SpaceExplorerPort", cocos2d::Rect(0, 0, designResolutionSize.width, designResolutionSize.height));
#else
        glview = GLViewImpl::create("SpaceExplorerPort");
#endif
        director->setOpenGLView(glview);
    }

    // set FPS. the default value is 1.0/60 if you don't call this
    director->setAnimationInterval(1.0 / 60);
    director->setProjection(Director::Projection::_2D);
    Texture2D::setDefaultAlphaPixelFormat(Texture2D::PixelFormat::AUTO);
    
    Device::setAccelerometerEnabled(false);
    
    //director->setDisplayStats(true);
    
    setDesignResolution(glview);
    setUserDefaults();
    SpriteFrameCache::getInstance()->addSpriteFramesWithFile("GameAssets.plist");
    SpriteFrameCache::getInstance()->addSpriteFramesWithFile("UIAssets.plist");
    
    auto tex = director->getTextureCache()->addImage("common/Background.png");
    if (!tex->hasMipmaps()) tex->generateMipmap();
    
    // load the user programs
    loadCustomGLPrograms();
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
    director->getEventDispatcher()->addCustomEventListener(EVENT_RENDERER_RECREATED, [] (EventCustom*) { loadCustomGLPrograms(); rebuildBlurPrograms(); });
    
    std::string pathToBGMusic = FileUtils::getInstance()->fullPathForFilename("BackgroundMusic.ogg");
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    GameCenterManager::authenticate([] { Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SocialManagersRefreshed"); });
    
    std::string pathToBGMusic = FileUtils::getInstance()->fullPathForFilename("BackgroundMusic.caf");
#endif

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	GPGManager::initialize();
#endif
    
    SoundManager::updateBackgroundVolume();
    CocosDenshion::SimpleAudioEngine::getInstance()->playBackgroundMusic(pathToBGMusic.c_str(), true);
    
    FacebookManager::initialize();
    ScoreManager::init();

    register_all_packages();

    // create a scene. it's an autorelease object
    auto scene = createSceneWithLayer(MultiPurposeLayer::createTitleScene(BackgroundColor));
    
    //if (!FacebookManager::hasPermission("user_friends"))
    //FacebookManager::requestReadPermissions();
    
    director->runWithScene(scene);

    return true;
}

// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();
    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("DidEnterBackground");
    
    CCLOG("Did enter background!");

    // if you use SimpleAudioEngine, it must be paused
    // SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    GameCenterManager::authenticate([] { Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SocialManagersRefreshed"); });
#endif
    
    Director::getInstance()->startAnimation();
    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("WillEnterForeground");
    
    CCLOG("Will enter foreground!");

    // if you use SimpleAudioEngine, it must resume here
    // SimpleAudioEngine::getInstance()->resumeBackgroundMusic();
}
