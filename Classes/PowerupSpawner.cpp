//
//  PowerupSpawner.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 03/05/15.
//
//

#include "PowerupSpawner.h"
#include "PlayerNode.h"
#include "CollisionManager.h"
#include "GameScene.h"
#include "SoundManager.h"
#include "AchievementManager.h"

using namespace cocos2d;

PowerupIcon* PowerupIcon::create(Sprite *originalSprite, float totalTime, const std::function<void()> &deactivateFunction)
{
    PowerupIcon *pRet = new(std::nothrow) PowerupIcon();
    
    if (pRet && pRet->init(originalSprite, totalTime, deactivateFunction))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

static constexpr float TotalAppearTime = 0.25;

bool PowerupIcon::init(Sprite *originalSprite, float totalTime, const std::function<void()> &deactivateFunction)
{
    if (!Sprite::initWithTexture(originalSprite->getTexture(), originalSprite->getTextureRect(), originalSprite->isTextureRectRotated()))
        return false;
    
    setGLProgram(GLProgramCache::getInstance()->getGLProgram("PowerupIconProgram"));
    
    getGLProgramState()->setUniformFloat("pixelSize", 1/16.0);
    getGLProgramState()->setUniformFloat("tolerance", 1.0);
    
    setScale(0);
    
    originalSprite->runAction(Sequence::createWithTwoActions(EaseQuadraticActionOut::create(ScaleTo::create(TotalAppearTime, 0)), RemoveSelf::create()));
    
    active = true;
    paused = false;
    
    this->deactivateFunction = deactivateFunction;
    this->totalTime = totalTime;
    currentTime = 0;
    
    scheduleUpdate();
    
    return true;
}

void PowerupIcon::update(float delta)
{
    if (paused) return;
    
    currentTime += delta;
    
    float fraction = std::min(currentTime/totalTime, 1.0f);
    
    // Build color
    float fractionUpper = 255*fraction;
    float fractionLower = 256*(fractionUpper - floorf(fractionUpper));
    
    setColor(Color3B(GLubyte(fractionUpper), GLubyte(fractionLower), 0));
    
    if (currentTime > totalTime)
    {
        deactivateFunction();
        collapse();
    }
}

void PowerupIcon::collapse(bool exploding)
{
    unscheduleUpdate();
    active = false;
    
    FiniteTimeAction *action;
    
    if (exploding) action = Spawn::createWithTwoActions(FadeOut::create(2*TotalAppearTime), ScaleTo::create(2*TotalAppearTime, 2.0));
    else action = EaseQuadraticActionOut::create(ScaleTo::create(TotalAppearTime, 0));
    
    runAction(Sequence::createWithTwoActions(action, RemoveSelf::create()));
}

void PowerupIcon::onEnterTransitionDidFinish()
{
    Sprite::onEnterTransitionDidFinish();
    paused = false;
}

void PowerupIcon::onExitTransitionDidStart()
{
    Sprite::onExitTransitionDidStart();
    paused = true;
}

bool PowerupSpawner::init()
{
    if (!Node::init())
        return false;
    
    spawnNumber = 0;
    auto action = CallFunc::create(CC_CALLBACK_0(PowerupSpawner::spawnPowerup, this));
    runAction(Sequence::createWithTwoActions(DelayTime::create(16), action));
    icons.reserve(3);
    
    listener = _eventDispatcher->addCustomEventListener("LifeUpdate", CC_CALLBACK_1(PowerupSpawner::detectDeath, this));
    
    scheduleUpdate();
    
    return true;
}

PowerupSpawner::~PowerupSpawner()
{
    _eventDispatcher->removeEventListener(listener);
}

void PowerupSpawner::onEnterTransitionDidFinish()
{
    Node::onEnterTransitionDidFinish();
    resume();
    for (Node *node : getChildren()) node->resume();
}

void PowerupSpawner::onExitTransitionDidStart()
{
    Node::onExitTransitionDidStart();
    pause();
    for (Node *node : getChildren()) node->pause();
}

void PowerupSpawner::onEnter()
{
    Node::onEnter();
    pause();
    for (Node *node : getChildren()) node->pause();
}

void PowerupSpawner::spawnPowerup()
{
    constexpr float MoveSpeed = 120;
    
    std::string spriteName;
    std::function<void(PlayerNode*, Sprite*)> func;
    
    int value = random(0, 20);
    switch (value)
    {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6:
            spriteName = "PowerupHeal.png";
            func = [] (PlayerNode* player, Sprite *sprite)
            {
                player->increaseHealth(10*random(1, 5));
                sprite->stopAllActions();
                sprite->runAction(Sequence::createWithTwoActions(EaseQuadraticActionOut::create(ScaleTo::create(0.25, 0)), RemoveSelf::create()));
            };
            break;
        case 7: case 8: case 9: case 10: case 11:
            spriteName = "PowerupShield.png";
            func = [this] (PlayerNode *player, Sprite *sprite)
            {
                sprite->stopAllActions();
                player->addShield();
                
                auto powerup = PowerupIcon::create(sprite, 30, [player] { player->removeShield(); });
                addIconToQueue(powerup, "ShieldIcon");
                player->changeShieldIcon(powerup);
            };
            break;
        case 12: case 13: case 14: case 15:
            spriteName = "PowerupDoubleScore.png";
            func = [this] (PlayerNode *player, Sprite *sprite)
            {
                bool value = true;
                auto eventDispatcher = _eventDispatcher;
                _eventDispatcher->dispatchCustomEvent("TriggerDoubleScore", &value);
                
                sprite->stopAllActions();
                addIconToQueue(PowerupIcon::create(sprite, 18, [eventDispatcher]
                {
                    bool value = false;
                    eventDispatcher->dispatchCustomEvent("TriggerDoubleScore", &value);
                }), "DoubleScoreIcon");
            };
            break;
        case 16: case 17: case 18: case 19:
            spriteName = "PowerupShooter.png";
            func = [this] (PlayerNode *player, Sprite *sprite)
            {
                sprite->stopAllActions();
                player->addShooter();
                
                addIconToQueue(PowerupIcon::create(sprite, 25, [player] { player->removeShooter(); }), "ShooterIcon");
            };
            break;
        case 20:
            spriteName = "PowerupAdvance.png";
            func = [this] (PlayerNode *player, Sprite *sprite)
            {
                _eventDispatcher->dispatchCustomEvent("TriggerAdvancePowerup");
                sprite->stopAllActions();
                player->turnInvincibility();
                
                addIconToQueue(PowerupIcon::create(sprite, 5, [] {}), "AdvanceIcon");
            };
        default: break;
            //CCASSERT(false, "PowerupSpawner switch should not branch to default! Adjust your random config!");
    }
    
    const Size &size = static_cast<GameScene*>(getParent()->getParent())->getPlayfieldSize();
    
    auto sprite = Sprite::create(spriteName);
    sprite->setPosition(size.width + 20, random<int>(40, size.height-40));
    sprite->setAnchorPoint(Vec2(0.5, 0.5));
    
    float distance = size.width + 40;
    sprite->runAction(Sequence::createWithTwoActions(MoveBy::create(distance/MoveSpeed, Vec2(-distance, 0)), RemoveSelf::create()));
    
    addChild(sprite);
    
    CollisionManager::addPowerup(CollisionManager::PowerupCollisionData(sprite, Vec2::ZERO, 16, [func, sprite] (const CollisionManager::PlayerCollisionData &data)
    {
		AchievementManager::increaseStat("PowerupCollected", 1);
        SoundManager::play("common/PowerupTaken.wav");
        func(static_cast<PlayerNode*>(data.positionNode), sprite);
    }));
    
    auto action = CallFunc::create(CC_CALLBACK_0(PowerupSpawner::spawnPowerup, this));
    runAction(Sequence::createWithTwoActions(DelayTime::create(16 + 0.75*(++spawnNumber)), action));
    
    if (!UserDefault::getInstance()->getBoolForKey("PowerupPresented"))
    {
        std::string str = "Collect powerups for some help";
        _eventDispatcher->dispatchCustomEvent("TutorialMessage", &str);
        
        UserDefault::getInstance()->setBoolForKey("PowerupPresented", true);
    }
}

void PowerupSpawner::addIconToQueue(PowerupIcon *icon, std::string name)
{
    PowerupIcon *secondIcon = nullptr;
    for (PowerupIcon *ic : icons)
        if (ic->getName() == name)
        {
            secondIcon = ic;
            break;
        }
    
    if (secondIcon) secondIcon->collapse();
    
    icon->setPosition(Vec2(-116, -28) + getScene()->getContentSize());
    icon->retain(); // Important!
    icon->setName(name);
    
    for (PowerupIcon *ic : icons) ic->runAction(MoveBy::create(TotalAppearTime, Vec2(-40, 0)));
    icons.pushBack(icon);
    icon->runAction(ScaleTo::create(TotalAppearTime, 1.0));
    
    getParent()->getParent()->addChild(icon);
}

void PowerupSpawner::update(float delta)
{
    for (auto it = icons.begin(); it != icons.end();)
    {
        if ((*it)->getParent() == nullptr)
        {
            for (auto it2 = icons.begin(); it2 != it; ++it2)
                if ((*it2)->active) (*it2)->runAction(MoveBy::create(TotalAppearTime, Vec2(40, 0)));
            
            (*it)->release();
            it = icons.erase(it);
        } else ++it;
    }
}

void PowerupSpawner::detectDeath(EventCustom *event)
{
    if (*static_cast<int*>(event->getUserData()) == 0)
        for (PowerupIcon *icon : icons) icon->collapse();
}