//
//  PowerupSpawner.h
//  SpaceExplorer
//
//  Created by João Baptista on 03/05/15.
//
//

#ifndef __SpaceExplorer__PowerupSpawner__
#define __SpaceExplorer__PowerupSpawner__

#include "cocos2d.h"

class PowerupSpawner;

class PowerupIcon : public cocos2d::Sprite
{
    std::function<void()> deactivateFunction;
    float currentTime, totalTime;
    bool paused;
    
    bool init(cocos2d::Sprite *originalSprite, float totalTime, const std::function<void()> &deactivateFunction);
public:
    virtual void update(float delta) override;
    
    virtual void onEnterTransitionDidFinish() override;
    virtual void onExitTransitionDidStart() override;
    
    void collapse(bool exploding = false);
    
    bool active;
    
    static PowerupIcon *create(cocos2d::Sprite *originalSprite, float totalTime, const std::function<void()> &deactivateFunction);
};

class PowerupSpawner : public cocos2d::Node
{
    int spawnNumber;
    cocos2d::Vector<PowerupIcon*> icons;
    cocos2d::EventListenerCustom* listener;
    
    virtual bool init() override;
    virtual void update(float delta) override;
    void spawnPowerup();
    void addIconToQueue(PowerupIcon *icon, std::string name);
    
    virtual void onEnterTransitionDidFinish() override;
    virtual void onExitTransitionDidStart() override;
    virtual void onEnter() override;
    
public:
    void detectDeath(cocos2d::EventCustom *custom);
    
    virtual ~PowerupSpawner();
    CREATE_FUNC(PowerupSpawner);
};

#endif /* defined(__SpaceExplorer__PowerupSpawner__) */
