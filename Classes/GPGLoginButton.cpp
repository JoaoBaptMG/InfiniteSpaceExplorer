//
//  GPGLoginButton.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 12/10/15.
//
//

#include "GPGLoginButton.h"
#include "GPGManager.h"
#include "MessageDialog.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

using namespace cocos2d;

bool GPGLoginButton::init()
{
	if (!Sprite::initWithSpriteFrameName("GPGSigningButton.png"))
		return false;

	touchListener = EventListenerTouchOneByOne::create();
	touchListener->onTouchBegan = CC_CALLBACK_2(GPGLoginButton::onTouchBegan, this);
	touchListener->onTouchMoved = CC_CALLBACK_2(GPGLoginButton::onTouchMoved, this);
	touchListener->onTouchEnded = CC_CALLBACK_2(GPGLoginButton::onTouchEnded, this);
	touchListener->onTouchCancelled = CC_CALLBACK_2(GPGLoginButton::onTouchCancelled, this);

	authorizationListener = _eventDispatcher->addCustomEventListener("GPGStatusUpdated", [=](EventCustom *custom)
	{
		refresh(GPGManager::isAuthorized());
	});

	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

	waitingForResponse = true;
	setPressed(false);

	return true;
}

GPGLoginButton::~GPGLoginButton()
{
	_eventDispatcher->removeEventListener(authorizationListener);
}

bool GPGLoginButton::onTouchBegan(Touch *touch, Event *event)
{
	if (getBoundingBox().containsPoint(getParent()->convertTouchToNodeSpaceAR(touch)))
	{
		setPressed(true);
		return true;
	}
	else return false;
}

void GPGLoginButton::onTouchMoved(Touch *touch, Event *event)
{
	setPressed(getBoundingBox().containsPoint(getParent()->convertTouchToNodeSpaceAR(touch)));
}

void GPGLoginButton::onTouchEnded(Touch *touch, Event *event)
{
	if (getBoundingBox().containsPoint(getParent()->convertTouchToNodeSpaceAR(touch)))
	{
		setPressed(false);
		act();
	}
}

void GPGLoginButton::onTouchCancelled(Touch *touch, Event *event)
{
	setPressed(false);
}

void GPGLoginButton::setPressed(bool pressed)
{
	setOpacity(waitingForResponse || !pressed ? 255 : 127);
}

void GPGLoginButton::refresh(bool isLoggedIn)
{
	waitingForResponse = false;
	setSpriteFrame(SpriteFrameCache::getInstance()->getSpriteFrameByName(!isLoggedIn ? "GPGSignInButton.png" : "GPGSignOutButton.png"));
}

void GPGLoginButton::act()
{
	if (waitingForResponse) return;

	if (GPGManager::isAuthorized())
	{
		presentMessage("Do you want to sign out from Google Play Games?", "Sign out?", "Yes", "No", [=]
		{ 
			waitingForResponse = true;
			setSpriteFrame(SpriteFrameCache::getInstance()->getSpriteFrameByName("GPGSigningButton.png"));
			GPGManager::signOut();
		}, [] {});
	}
	else
	{
		waitingForResponse = true;
		setSpriteFrame(SpriteFrameCache::getInstance()->getSpriteFrameByName("GPGSigningButton.png"));
		GPGManager::signIn();
	}
}

#endif
