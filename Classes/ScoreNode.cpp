//
//  ScoreNode.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 02/03/15.
//
//

#include "ScoreNode.h"
#include "Defaults.h"
#include "ScoreManager.h"
#include "SoundManager.h"

using namespace cocos2d;

float global_MaxMultiplier = 0;

TextUpdateAction* TextUpdateAction::create(unsigned long oldScore, unsigned long newScore, float duration)
{
    TextUpdateAction *pRet = new(std::nothrow) TextUpdateAction();
    if (pRet && pRet->init(oldScore, newScore, duration))
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

ScoreNode* ScoreNode::create(const Size &screenSize)
{
    ScoreNode *pRet = new(std::nothrow) ScoreNode();
    if (pRet && pRet->init(screenSize))
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

bool TextUpdateAction::init(unsigned long oldScore, unsigned long newScore, float duration)
{
    if (!ActionInterval::initWithDuration(duration))
        return false;
    
    this->oldScore = oldScore;
    this->newScore = newScore;
    curScore = oldScore;
    
    return true;
}

void TextUpdateAction::update(float time)
{
    curScore = oldScore + time*(newScore-oldScore);
    
    auto text = static_cast<Label*>(getTarget());
    
    auto lastPos = text->getPosition() + text->getContentSize()/2;
    text->setString(ulongToString(curScore, 6));
    text->setPosition(lastPos - text->getContentSize()/2);
}

constexpr float MultiplierUpgradeTime = 21, MaxMultiplierUpgrade = 9, MultiplierUpgradeStep = 0.5;

long global_GameScore = 0;

bool ScoreNode::init(const Size &screenSize)
{
    if (!Node::init())
        return false;
    
        scheduleUpdate();
    
    global_GameScore = 0;
    global_MaxMultiplier = 1;
    multiplier = 1;
    multiplierTime = INFINITY;
    
    scoreText = Label::createWithSystemFont("000000", "fonts/Lato/Lato-Bold.ttf", 24);
    multiplierText = Label::createWithSystemFont("1.0x", "fonts/Lato/Lato-Light.ttf", 12);
    
    scoreText->setHorizontalAlignment(TextHAlignment::RIGHT);
    scoreText->setVerticalAlignment(TextVAlignment::TOP);
    
    scoreTrackingText = nullptr;
    
    addChild(scoreText);
    addChild(multiplierText);
    computeTextPositions(screenSize);
    
    nextTrackedScore = 0;
    
    playerDead = paused = false;
    
    scoreUpdateListener = _eventDispatcher->addCustomEventListener("ScoreUpdate", CC_CALLBACK_1(ScoreNode::updateScore, this));
    lifeUpdateListener = _eventDispatcher->addCustomEventListener("LifeUpdate", CC_CALLBACK_1(ScoreNode::lifeUpdate, this));
    
    doubleScoreListener = _eventDispatcher->addCustomEventListener("TriggerDoubleScore", [this] (EventCustom *event)
    {
        doubleScore = *static_cast<bool*>(event->getUserData());
        updateMultiplierText(doubleScore);
    });
    
    dummyListener = _eventDispatcher->addCustomEventListener("HazardSelectorAvailable", [this] (EventCustom *event)
    {
        multiplierTime = MultiplierUpgradeTime;
        _eventDispatcher->removeEventListener(dummyListener);
    });
    
    textUpdateAction = nullptr;
    
    resetScore();
    setName("ScoreNode");
    
    ScoreManager::updateScoreTrackingArray();
    
    return true;
}

ScoreNode::~ScoreNode()
{
    _eventDispatcher->removeEventListener(scoreUpdateListener);
    _eventDispatcher->removeEventListener(lifeUpdateListener);
    _eventDispatcher->removeEventListener(doubleScoreListener);
}

void ScoreNode::onEnterTransitionDidFinish()
{
    Node::onEnterTransitionDidFinish();
    recursiveResume(this);
    paused = false;
}

void ScoreNode::onExitTransitionDidStart()
{
    Node::onExitTransitionDidStart();
    recursivePause(this);
    paused = true;
}

void ScoreNode::onEnter()
{
    Node::onEnter();
    recursivePause(this);
    paused = true;
}

void ScoreNode::computeTextPositions(const Size &size)
{
    auto pos = Vec2(size) - Vec2(12, 12);
    auto ssize = scoreText->getContentSize();
    auto msize = multiplierText->getContentSize();
    
    auto spos = pos + Vec2(0, 8 - msize.height);
    
    scoreText->setPosition(spos - ssize/2);
    multiplierText->setPosition(Vec2(pos.x - ssize.width + msize.width/2, pos.y - msize.height/2 + 2));
}

void ScoreNode::update(float delta)
{
    if (!paused && !playerDead) multiplierTime -= delta;
    
    if (multiplierTime <= 0)
    {
        multiplier += MultiplierUpgradeStep;
        global_MaxMultiplier = MAX(global_MaxMultiplier, multiplier);
        multiplierTime = MultiplierUpgradeTime + 2*multiplier;
        updateMultiplierText();
    }
    
    updateScoreTracking();
}

void ScoreNode::updateMultiplierText(bool createText)
{
    float realMultiplier = doubleScore ? 2*multiplier : multiplier;
    
    int multInt = realMultiplier;
    int multFrac = int(realMultiplier*10) - multInt*10;

    auto lastPos = multiplierText->getPosition() - multiplierText->getContentSize()/2;
    multiplierText->setString(ulongToString(multInt) + "." + ulongToString(multFrac) + "x" + (doubleScore ? " (+)" : ""));
    multiplierText->setPosition(lastPos + multiplierText->getContentSize()/2);
    
    auto step = (MIN(multiplier, MaxMultiplierUpgrade)-1)/(MaxMultiplierUpgrade-1);
    multiplierText->setTextColor(Color4B(255, 255 - 41*step, 255*(1-step), 255));
    
    if (createText)
    {
        removeChildByName("MultiplierText");
        
        auto size = getScene()->getContentSize();
        
        auto text = Label::createWithSystemFont("Score multiplier: " + multiplierText->getString() + "!", "fonts/Lato/Lato-Regular.ttf", 24);
        text->setPosition(size/2);
        
        text->setName("MultiplierText");
        
        text->setTextColor(Color4B(255, 255 - 41*step, 255*(1-step), 255));
        text->setOpacity(0);
        
        auto fade = Sequence::create(FadeIn::create(0.2), DelayTime::create(1.4), FadeOut::create(0.2), RemoveSelf::create(), nullptr);
        
        text->runAction(fade);
        
        addChild(text);
        SoundManager::play("common/MultiplierUpgrade.wav");
    }
}

void ScoreNode::lifeUpdate(EventCustom *event)
{
    if (*static_cast<int*>(event->getUserData()) >= 0)
    {
        multiplier = 1;
        multiplierTime = MultiplierUpgradeTime;
        updateMultiplierText(false);
    }
    
    if (*static_cast<int*>(event->getUserData()) == 0) playerDead = true;
}

void ScoreNode::updateScore(EventCustom *event)
{
    if (playerDead) return;
    
    float realMultiplier = doubleScore ? 2*multiplier : multiplier;
    
    long newScore = global_GameScore + realMultiplier * *static_cast<int*>(event->getUserData());
    long oldScore;
    
    if (!textUpdateAction || textUpdateAction->isDone())
        oldScore = global_GameScore;
    else
    {
        oldScore = textUpdateAction->curScore;
        scoreText->stopAction(textUpdateAction);
    }
    
    auto time = cbrtf(newScore - oldScore)/20;
    
    if (textUpdateAction) textUpdateAction->release();
    global_GameScore = newScore;
    scoreText->runAction(textUpdateAction = TextUpdateAction::create(oldScore, newScore, time));
    textUpdateAction->retain();
}

void ScoreNode::updateScoreTracking()
{
    int64_t score = textUpdateAction == nullptr ? 0 : textUpdateAction->curScore;
    
    if (ScoreManager::trackedScoresReady() && score >= nextTrackedScore)
    {
        ScoreManager::ScoreData data = ScoreManager::getNextTrackedScore(score);
        
        if (scoreTrackingText != nullptr)
        {
            scoreTrackingText->runAction(Sequence::createWithTwoActions(MoveBy::create(0.4, Vec2(0, 12)), RemoveSelf::create()));
            scoreTrackingText->runAction(FadeOut::create(0.4));
        }
        
        if (data.index > -1)
        {
            auto newLabel = Label::createWithSystemFont(data.name + ": " + longToString(data.score, 6), "fonts/Lato/Lato-Regular.ttf", 16);
            newLabel->setPosition(getScene()->getContentSize().width/2, getScene()->getContentSize().height - 32);
            newLabel->setTextColor(data.isPlayer ? Color4B::YELLOW : Color4B::WHITE);
            addChild(newLabel);
        
            newLabel->setOpacity(0);
            newLabel->runAction(MoveBy::create(0.4, Vec2(0, 12)));
            newLabel->runAction(FadeIn::create(0.4));
        
            nextTrackedScore = data.score;
            scoreTrackingText = newLabel;
        }
        else scoreTrackingText = nullptr;
    }
}

void ScoreNode::resetScore()
{
    global_GameScore = 0;
    multiplier = 1;
    
    nextTrackedScore = 0;
    scoreText->setString("000000");
    updateMultiplierText(false);
}
