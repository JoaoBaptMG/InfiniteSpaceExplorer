//
//  TutorialNode.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 14/03/15.
//
//

#include "TutorialNode.h"
#include "Defaults.h"

constexpr float FadeTime = 0.3;
constexpr float Duration = 3.0;
constexpr float AcDuration = Duration - 2*FadeTime;

using namespace cocos2d;

TutorialNode* TutorialNode::create(const Size &screenSize)
{
    TutorialNode *pRet = new(std::nothrow) TutorialNode();
    if (pRet && pRet->init(screenSize))
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

bool TutorialNode::init(const Size &screenSize)
{
    if (!ui::Text::init("", "fonts/Lato/Lato-Regular.ttf", 24))
        return false;
    
    setTextHorizontalAlignment(TextHAlignment::CENTER);
    setTextVerticalAlignment(TextVAlignment::CENTER);
    
    setOpacity(0);
    setPosition(Vec2(screenSize.width/2, screenSize.height * 0.2));
    
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(false);
    touchListener->onTouchBegan = [this] (Touch *touch, Event *Event) { return waitForInput; };
    touchListener->onTouchEnded = CC_CALLBACK_2(TutorialNode::tapScreen, this);
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    tutorialMessageListener = _eventDispatcher->addCustomEventListener("TutorialMessage", CC_CALLBACK_1(TutorialNode::pushMessage, this));
    
    setName("TutorialNode");
    scheduleUpdate();
    
    return true;
}

TutorialNode::~TutorialNode()
{
    _eventDispatcher->removeEventListener(tutorialMessageListener);
}

void TutorialNode::onEnterTransitionDidFinish()
{
    Node::onEnterTransitionDidFinish();
    recursiveResume(this);
}

void TutorialNode::onEnter()
{
    Node::onEnter();
    recursivePause(this);
}

void TutorialNode::onExitTransitionDidStart()
{
    Node::onExitTransitionDidStart();
    recursivePause(this);
}

void TutorialNode::pushMessage(EventCustom *event)
{
    auto prevEmpty = messageQueue.empty();
    messageQueue.push_back(*static_cast<std::string*>(event->getUserData()));
    if (prevEmpty) processMessage();
}

void TutorialNode::nextMessage()
{
    if (!queuedEvent.empty())
    {
        _eventDispatcher->dispatchCustomEvent(queuedEvent);
        queuedEvent = "";
    }
    
    messageQueue.pop_front();
    processMessage();
}

void TutorialNode::processMessage()
{
    if (messageQueue.empty()) return;
    
    auto message = messageQueue.front();
    if (message[0] == '#' && message[1] == '{')
    {
        auto range = message.find_first_of('}', 2);
        if (range == std::string::npos) return nextMessage();
        queuedEvent = message.substr(2, range-2);
        message.erase(0, range+1);
    }
    
    if (message[0] == '$')
    {
        setString(message.substr(1));
        runAction(Sequence::create(FadeIn::create(FadeTime), CallFunc::create([this] { waitForInput = true; }), nullptr));
    }
    else
    {
        setString(message);
        runAction(Sequence::create(FadeIn::create(FadeTime), DelayTime::create(AcDuration), FadeOut::create(FadeTime),
                                   CallFunc::create(CC_CALLBACK_0(TutorialNode::nextMessage, this)), nullptr));
    }
}

void TutorialNode::tapScreen(Touch *touch, Event *event)
{
    if (waitForInput)
    {
        waitForInput = false;
        runAction(Sequence::create(FadeOut::create(FadeTime), CallFunc::create(CC_CALLBACK_0(TutorialNode::nextMessage, this)), nullptr));
    }
}