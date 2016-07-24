//
//  TutorialNode.h
//  SpaceExplorer
//
//  Created by João Baptista on 14/03/15.
//
//

#ifndef __SpaceExplorer__TutorialNode__
#define __SpaceExplorer__TutorialNode__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class TutorialNode : public cocos2d::ui::Text
{
    std::deque<std::string> messageQueue;
    std::string queuedEvent;
    bool waitForInput;
    
    cocos2d::EventListenerCustom *tutorialMessageListener;
    
    void pushMessage(cocos2d::EventCustom* event);
    void nextMessage();
    void processMessage();
    
    void tapScreen(cocos2d::Touch* touch, cocos2d::Event* event);
    
    bool init(const cocos2d::Size &screenSize);
    
public:
    virtual void onEnterTransitionDidFinish();
    virtual void onEnter();
    virtual void onExitTransitionDidStart();
    
    static TutorialNode *create(const cocos2d::Size &screenSize);
    
    virtual ~TutorialNode();
};

#endif /* defined(__SpaceExplorer__TutorialNode__) */
