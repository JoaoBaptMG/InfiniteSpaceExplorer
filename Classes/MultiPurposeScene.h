//
//  MultiPurposeScene.h
//  SpaceExplorer
//
//  Created by João Baptista on 28/02/15.
//
//

#ifndef __SpaceExplorer__MultiPurposeScene__
#define __SpaceExplorer__MultiPurposeScene__

#include "ui/CocosGUI.h"
#include "cocos2d.h"

class MultiPurposeLayer : public cocos2d::LayerColor
{
    bool init(cocos2d::Color3B color, bool pause);
    static MultiPurposeLayer *create(cocos2d::Color3B color, bool pause);
    
    void createLayout1Pause();
    void createLayout1Title();
    void createLayout2();
    void createLayout3();
    
    void changeLayout(int nextLayout);
    void popToGame();
    void exitConfirm();
    
    virtual void onEnterTransitionDidFinish() override;
    
    cocos2d::Node *containerNode;
    int currentLayout;
    cocos2d::ui::Button *arrowSprites[2];
    
public:
    inline static MultiPurposeLayer *createTitleScene(cocos2d::Color3B color) { return create(color, false); }
    inline static MultiPurposeLayer *createPauseScene(cocos2d::Color3B color) { return create(color, true); }
    
    ~MultiPurposeLayer();
};

#endif /* defined(__SpaceExplorer__MultiPurposeScene__) */
