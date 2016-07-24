//
//  ScoreNode.h
//  SpaceExplorer
//
//  Created by João Baptista on 02/03/15.
//
//

#ifndef __SpaceExplorer__ScoreNode__
#define __SpaceExplorer__ScoreNode__

#include "cocos2d.h"

class TextUpdateAction : public cocos2d::ActionInterval
{
    friend class ScoreNode;
    
    unsigned long newScore, oldScore, curScore;
    bool init(unsigned long oldScore, unsigned long newScore, float duration);
    
public:
    virtual void update(float time) override;
    
    static TextUpdateAction *create(unsigned long oldScore, unsigned long newScore, float duration);
};

class ScoreNode : public cocos2d::Node
{
    cocos2d::Label *scoreText, *multiplierText, *scoreTrackingText;
    cocos2d::EventListenerCustom *scoreUpdateListener, *lifeUpdateListener, *dummyListener, *doubleScoreListener;
    
    bool doubleScore;
    float multiplier, multiplierTime;
    TextUpdateAction *textUpdateAction;
    
    void computeTextPositions(const cocos2d::Size &size);
    void updateMultiplierText(bool createText = true);
    void lifeUpdate(cocos2d::EventCustom *event);
    void updateScore(cocos2d::EventCustom *event);
    void updateScoreTracking();
    int64_t nextTrackedScore;
    
    bool playerDead, paused;
    
    bool init(const cocos2d::Size &screenSize);
    
public:
    virtual void onEnterTransitionDidFinish() override;
    virtual void onExitTransitionDidStart() override;
    virtual void onEnter() override;
    
    virtual void update(float delta) override;
    void resetScore();
    
    static ScoreNode *create(const cocos2d::Size &screenSize);

    virtual ~ScoreNode();
};

#endif /* defined(__SpaceExplorer__ScoreNode__) */
