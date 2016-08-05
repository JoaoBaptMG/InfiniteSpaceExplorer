//
//  FacebookLoginButton.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 12/10/15.
//
//

#include "FacebookLoginButton.h"
#include "FacebookManager.h"
#include "MessageDialog.h"

using namespace cocos2d;

bool FacebookLoginButton::init()
{
    if (!Sprite::initWithSpriteFrameName("FacebookLoginButton.png"))
        return false;
    
    touchListener = EventListenerTouchOneByOne::create();
    touchListener->onTouchBegan = CC_CALLBACK_2(FacebookLoginButton::onTouchBegan, this);
    touchListener->onTouchMoved = CC_CALLBACK_2(FacebookLoginButton::onTouchMoved, this);
    touchListener->onTouchEnded = CC_CALLBACK_2(FacebookLoginButton::onTouchEnded, this);
    touchListener->onTouchCancelled = CC_CALLBACK_2(FacebookLoginButton::onTouchCancelled, this);
    
    accessTokenListener = _eventDispatcher->addCustomEventListener("FacebookUpdated", [=] (EventCustom *custom)
    {
        refresh(*static_cast<bool*>(custom->getUserData()));
    });
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    
    waitingForResponse = false;
    setPressed(false);
    
    label = Label::createWithTTF("", "fonts/Lato/Lato-Regular.ttf", 18);
    label->setPosition(82, 16);
    label->setTextColor(Color4B::WHITE);
    addChild(label);
    
    refresh(FacebookManager::isAccessTokenValid());
    
    return true;
}

FacebookLoginButton::~FacebookLoginButton()
{
    _eventDispatcher->removeEventListener(accessTokenListener);
}

bool FacebookLoginButton::onTouchBegan(Touch *touch, Event *event)
{
    if (getBoundingBox().containsPoint(getParent()->convertTouchToNodeSpaceAR(touch)))
    {
        setPressed(true);
        return true;
    }
    else return false;
}

void FacebookLoginButton::onTouchMoved(Touch *touch, Event *event)
{
    setPressed(getBoundingBox().containsPoint(getParent()->convertTouchToNodeSpaceAR(touch)));
}

void FacebookLoginButton::onTouchEnded(Touch *touch, Event *event)
{
    if (getBoundingBox().containsPoint(getParent()->convertTouchToNodeSpaceAR(touch)))
    {
        setPressed(false);
        act();
    }
}

void FacebookLoginButton::onTouchCancelled(Touch *touch, Event *event)
{
    setPressed(false);
}

void FacebookLoginButton::setPressed(bool pressed)
{
    setOpacity(waitingForResponse || !pressed ? 255 : 127);
}

void FacebookLoginButton::refresh(bool isLoggedIn)
{
    waitingForResponse = false;
    label->setString(!isLoggedIn ? "LOG IN" : "LOG OUT");
}

void FacebookLoginButton::act()
{
    if (waitingForResponse) return;
    
    if (FacebookManager::isAccessTokenValid())
    {
        presentMessage("Do you want to log out from Facebook? You won't be able to publish your scores this way.", "Log out?", "Yes", "No",
                       [=] { label->setString("..."); waitingForResponse = true; FacebookManager::logOut(); }, [] {});
    }
    else
    {
		waitingForResponse = true;
        label->setString("...");
        FacebookManager::requestReadPermissions([=] (FacebookManager::PermissionState state, std::string)
        {
            if (state == FacebookManager::PermissionState::ERROR) label->setString("ERROR");
            else
            {
                refresh(state == FacebookManager::PermissionState::ACCEPTED);
            }
        }, true);
    }
}
