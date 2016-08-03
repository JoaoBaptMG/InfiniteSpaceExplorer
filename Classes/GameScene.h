//
//  GameScene.h
//  SpaceExplorer
//
//  Created by João Baptista on 07/03/15.
//
//

#ifndef __SpaceExplorer__GameScene__
#define __SpaceExplorer__GameScene__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class GameScene : public cocos2d::LayerColor
{
    bool init();
    
    int colorID;
    float gameTime;
    cocos2d::Node *gameLayer, *uiLayer, *backgroundLayer;
    cocos2d::EventListenerCustom *lifeUpdateListener, *tutorialDoneListener;
    cocos2d::EventListenerCustom *backgroundListener, *foregroundListener;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    cocos2d::EventListenerCustom *socialEventListener;
#endif
    
    cocos2d::RenderTexture* firstRenderTexture;
    cocos2d::RenderTexture* targetRenderTexture;
    cocos2d::RenderTexture* helperRenderTexture;
    cocos2d::RenderTexture* outputRenderTexture;
    
    cocos2d::Size playfieldSize;
    
    bool alreadyChecked, alreadyChecked2;
    
    void lifeUpdate(cocos2d::EventCustom *event);
    void checkTutorialPhase();
    cocos2d::ui::Button *createPauseButton();
    
    void pauseButtonPressed(cocos2d::Ref *object, cocos2d::ui::Widget::TouchEventType type);
    
    void toBackground(cocos2d::EventCustom *event);
    void toForeground(cocos2d::EventCustom *event);
    
    void gotoPauseScreen();
    
public:
    virtual void onEnter() override;
    virtual void onExitTransitionDidStart() override;
    virtual void onEnterTransitionDidFinish() override;
    virtual void update(float delta) override;
    
    virtual void visit(cocos2d::Renderer *renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) override;
    
    const cocos2d::Size &getPlayfieldSize() const { return playfieldSize; }
    
    void reset();
    
    CREATE_FUNC(GameScene);
    
    virtual ~GameScene();
};

#endif /* defined(__SpaceExplorer__GameScene__) */
