//
//  HazardSelector.h
//  SpaceExplorer
//
//  Created by João Baptista on 16/03/15.
//
//

#ifndef __SpaceExplorer__HazardSelector__
#define __SpaceExplorer__HazardSelector__

#include "cocos2d.h"
#include "Defaults.h"

enum class ExitDir : int
{
    LEFT = 1,
    RIGHT = 2,
    TOP = 4,
    BOTTOM = 8,
    ALL = 15
};
inline static ExitDir operator|(ExitDir i1, ExitDir i2) { return ExitDir((int)i1 | (int)i2); }
inline static ExitDir operator&(ExitDir i1, ExitDir i2) { return ExitDir((int)i1 & (int)i2); }

class HazardSelector : public cocos2d::Node
{
    bool paused;
    std::list<cocos2d::Speed*> runningActions;
    std::vector<std::function<void(float)>> runningScheduledFunctions;
    std::vector<float> actualProbabilities;
    cocos2d::EventListenerCustom *triggerAdvancePowerupListener;
    bool isArtificialAdvance;
    
    int currentChosen;
    float enterDelay;
    
    bool init(bool onTitle = false);
    
public:
    virtual ~HazardSelector();
    
    virtual void update(float delta) override;
    virtual void onExitTransitionDidStart() override;
    virtual void onEnterTransitionDidFinish() override;
    
    void triggerAdvancePowerup(cocos2d::EventCustom *event);
    void presentHazard(int id);
    void moveHazardOffscreen(cocos2d::RefPtr<cocos2d::Node> node, cocos2d::Vec2 dir, cocos2d::RefPtr<cocos2d::Action> nextAction, ExitDir exitDir = ExitDir::ALL, float expansion = 0);
    
    float spawnTime, currentTime, gameTime;
    float speed;
    
    bool onTitle;
    
    inline void artificiallyAdvance(int numFrames)
    {
        isArtificialAdvance = true;
        while (numFrames--)
        {
            update(0.016f);
            for (auto action : runningActions) action->step(0.016f);
            for (auto function : runningScheduledFunctions) function(0.016f);
        }
        isArtificialAdvance = false;
        runningScheduledFunctions.clear();
    }
    
    inline cocos2d::Vec2 getPlayerPos(cocos2d::Vec2 def)
    {
        auto player = getParent()->getChildByName("PlayerNode");
        if (player) return player->getPosition();
        return def;
    }
    
    inline cocos2d::Speed* addAction(cocos2d::Action* action)
    {
        auto result = cocos2d::Speed::create(action, speed);
        result->retain();
        runningActions.push_back(result);
        return result;
    }
    
    inline cocos2d::Size getPlayfieldSize()
    {
        auto size = _director->getWinSize();
        return cocos2d::Size(size.width/size.height * StandardPlayfieldHeight, StandardPlayfieldHeight);
    }
    
    static HazardSelector* create(bool onTitle = false)
    {
        HazardSelector *pRet = new(std::nothrow) HazardSelector();
        if (pRet && pRet->init(onTitle))
        {
            pRet->autorelease();
            return pRet;
        }
        else
        {
            delete pRet;
            pRet = NULL;
            return NULL;
        }
    }
};

inline static int random_int_open(int start, int end)
{
    if (start > end) std::swap(start, end);
    return cocos2d::random<int>(start, end-1);
}

inline static int random_int_closed(int start, int end)
{
    if (start > end) std::swap(start, end);
    return cocos2d::random<int>(start, end);
}

inline static float random_float_open(float start, float end)
{
    if (start > end) std::swap(start, end);
    return cocos2d::random<float>(start, end);
}

inline static float random_float_closed(float start, float end)
{
    if (start > end) std::swap(start, end);
    return cocos2d::random<float>(start, std::nextafter(end, end+1));
}

extern const struct HazardSpawner { void (*function)(HazardSelector*); float probability, entranceTime, enterDelay; } hazardSpawners[];
extern const std::string hazardStrings[];
extern const int hazardSpawnerSize;

#endif /* defined(__SpaceExplorer__HazardSelector__) */
