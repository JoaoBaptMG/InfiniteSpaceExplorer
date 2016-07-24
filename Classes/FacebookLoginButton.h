//
//  FacebookLoginButton.hpp
//  SpaceExplorer
//
//  Created by João Baptista on 12/10/15.
//
//

#ifndef FacebookLoginButton_hpp
#define FacebookLoginButton_hpp

#include "cocos2d.h"

class FacebookLoginButton : public cocos2d::Sprite
{
    cocos2d::EventListenerTouchOneByOne *touchListener;
    cocos2d::EventListenerCustom *accessTokenListener;
    
    cocos2d::Label *label;
    
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
    virtual ~FacebookLoginButton();
    
    CREATE_FUNC(FacebookLoginButton);
};

#endif /* FacebookLoginButton_hpp */
