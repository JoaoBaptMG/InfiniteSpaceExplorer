//
//  HazardSelector.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 16/03/15.
//
//

#include "HazardSelector.h"
#include "CustomActions.h"
#include "GameScene.h"
#include "AchievementManager.h"

using namespace cocos2d;

float global_AdvanceSpeed = 0.0f;
float global_GameTime = 0.0f;

bool HazardSelector::init(bool onTitle)
{
    if (!Node::init())
        return false;
    
    spawnTime = 5.0;
    currentTime = 0.0;
	global_GameTime = 0.0;
    speed = 1.0;
    global_AdvanceSpeed = 0.0f;
    
    enterDelay = -1.0;
    currentChosen = -1;
    
    this->onTitle = onTitle;
    isArtificialAdvance = false;
    
    actualProbabilities.resize(hazardSpawnerSize);
    for (int i = 0; i < hazardSpawnerSize; i++)
        actualProbabilities[i] = hazardSpawners[i].probability;
    paused = false;
    
    setName("HazardSelector");
    
    _eventDispatcher->dispatchCustomEvent("HazardSelectorAvailable");
    triggerAdvancePowerupListener = _eventDispatcher->addCustomEventListener("TriggerAdvancePowerup", CC_CALLBACK_1(HazardSelector::triggerAdvancePowerup, this));
    
    scheduleUpdate();
    
    return true;
}

void HazardSelector::update(float delta)
{
    if (paused) return;
    
    speed = 1 + currentTime/1080;
    if (onTitle) speed *= 0.5;
    currentTime += delta * speed;
    
	if (!onTitle)
	{
		int oldMin = global_GameTime / 60;
		global_GameTime += delta;
		int newMin = global_GameTime / 60;

		if (oldMin != newMin) AchievementManager::updateStat("GameTime", newMin);
	}

    for (auto it = runningActions.begin(); it != runningActions.end();)
    {
        if ((*it)->isDone())
        {
            (*it)->release();
            it = runningActions.erase(it);
        }
        else (*it++)->setSpeed(speed);
    }
    
    if (global_AdvanceSpeed > 0)
        for (Node *value : getChildren())
            value->setPositionX(value->getPositionX() - delta * global_AdvanceSpeed);
    
    if (enterDelay >= 0)
    {
        enterDelay -= delta * speed * (1 + global_AdvanceSpeed/150);
        
        if (enterDelay <= 0)
        {
            log("Hazard spawned: %d at time %g!", currentChosen, currentTime);
            
            hazardSpawners[currentChosen].function(this);
            presentHazard(currentChosen);
        }
    }
    else if (spawnTime >= 0) spawnTime -= delta * speed * (1 + global_AdvanceSpeed/150);
    else
    {
        float totalProb = 0.0f;
        for (int i = 0; i < hazardSpawnerSize; i++)
            if (hazardSpawners[i].entranceTime < currentTime)
                totalProb += hazardSpawners[i].probability;
        
        auto rand = random<float>(0, totalProb);
        float weight = 0.0f;
        int current = 0;
        
        for (int i = 0; i < hazardSpawnerSize; i++)
        {
            if (!(hazardSpawners[i].entranceTime < currentTime)) continue;
            current = i;
            weight += actualProbabilities[i];
            if (weight > rand) break;
        }
        
        // Update spawns
        int count = 0;
        auto spawnWeight = powf(random_float_closed(0,1), 480/currentTime) * 0.8;
        
        for (int i = 0; i < hazardSpawnerSize; i++)
        {
            if (i == current || !(hazardSpawners[i].entranceTime < currentTime)) continue;
            actualProbabilities[i] += actualProbabilities[current] * spawnWeight * hazardSpawners[i].probability / (totalProb - hazardSpawners[current].probability);
            count++;
        }
        if (count > 0) actualProbabilities[current] *= 1 - spawnWeight;
        
        enterDelay = hazardSpawners[current].enterDelay * expf(-currentTime/600);
        currentChosen = current;
    }
}

void HazardSelector::onExitTransitionDidStart()
{
    Node::onExitTransitionDidStart();
    paused = true;
}

void HazardSelector::onEnterTransitionDidFinish()
{
    Node::onEnterTransitionDidFinish();
    paused = false;
}

HazardSelector::~HazardSelector()
{
    for (auto action : runningActions) action->release();
    _eventDispatcher->removeEventListener(triggerAdvancePowerupListener);
}

void HazardSelector::triggerAdvancePowerup(EventCustom*)
{
    auto slow = ExecFunc::create(1.0, [](Node *node, float time) { global_AdvanceSpeed = 1500*time; });
    auto speed = ExecFunc::create(1.0, [](Node *node, float time) { global_AdvanceSpeed = 1500*(1 - time); });
    runAction(Sequence::create(slow, DelayTime::create(4.0), speed, nullptr));
    
    auto size = getPlayfieldSize();
    auto dimScreen = LayerColor::create(Color4B(0, 0, 0, 255), size.width, size.height);
    dimScreen->setOpacity(0);
    getParent()->addChild(dimScreen, -4);
    
    dimScreen->runAction(Sequence::create(FadeTo::create(1.0, 128), DelayTime::create(4.0), FadeTo::create(1.0, 0), RemoveSelf::create(), nullptr));
}

void HazardSelector::presentHazard(int id)
{
    if (onTitle) return;
    if (getParent()->getChildByName("PlayerNode") == nullptr) return;
    
    int index = UserDefault::getInstance()->getIntegerForKey("PresentedHazards");
    if ((index & (1 << id)) == 0)
    {
        std::string str = hazardStrings[id];
        _eventDispatcher->dispatchCustomEvent("TutorialMessage", &str);
        UserDefault::getInstance()->setIntegerForKey("PresentedHazards", index | (1 << id));
    }
}

void HazardSelector::moveHazardOffscreen(RefPtr<Node> node, Vec2 dir, RefPtr<Action> nextAction, ExitDir exitDir, float expansion)
{
    //RefPtr<Action> nextActionPtr = nextAction;
    
    auto schedule = [=] (float dt)
    {
        if (node->getParent() == nullptr) return;
        node->setPosition(node->getPosition() + dt * speed * dir);
        
        const Size &screenSize = getPlayfieldSize();
        
        // If the rect that contains the entire screen doesn't intersect with the rect that contains the object (transformed to world coordinates),
        // the object is offscreen and the action is done
        
        Rect screenRect(-expansion, -expansion, screenSize.width + 2*expansion, screenSize.height + 2*expansion);
		Rect transformedObjectRect = node->getBoundingBox();
        
        // Use a special intersection algorithm
        bool intersects = true;
        if (int(exitDir & ExitDir::LEFT) && transformedObjectRect.getMaxX() < screenRect.getMinX()) intersects = false;
        if (int(exitDir & ExitDir::RIGHT) && transformedObjectRect.getMinX() > screenRect.getMaxX()) intersects = false;
        if (int(exitDir & ExitDir::BOTTOM) && transformedObjectRect.getMaxY() < screenRect.getMinY()) intersects = false;
        if (int(exitDir & ExitDir::TOP) && transformedObjectRect.getMinY() > screenRect.getMaxY()) intersects = false;
        
        if (!intersects)
            node->runAction(addAction(nextAction));
    };
    
    if (isArtificialAdvance) runningScheduledFunctions.push_back(schedule);
    node->schedule(schedule, "MoveOffscreen");
}