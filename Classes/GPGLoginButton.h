//
//  GPGLoginButton.h
//  SpaceExplorer
//
//  Created by João Baptista on 05/08/16.
//
//

#ifndef GPGLoginButton_hpp
#define GPGLoginButton_hpp

#include "cocos2d.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

class GPGLoginButton : public cocos2d::Sprite
{
	cocos2d::EventListenerTouchOneByOne *touchListener;
	cocos2d::EventListenerCustom *authorizationListener;

	bool onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event);
	void onTouchMoved(cocos2d::Touch *touch, cocos2d::Event *event);
	void onTouchEnded(cocos2d::Touch *touch, cocos2d::Event *event);
	void onTouchCancelled(cocos2d::Touch *touch, cocos2d::Event *event);

	void setPressed(bool pressed);
	void refresh(bool isLoggedIn);

	void act();

	bool waitingForResponse;

public:
	bool init();
	virtual ~GPGLoginButton();

	CREATE_FUNC(GPGLoginButton);
};

#endif

#endif /* GPGLoginButton_hpp */
