//
//  LifeMarker.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 13/03/15.
//
//

#include "LifeMarker.h"
#include "PlayerNode.h" // for MaxHealth
#include "Defaults.h"

using namespace cocos2d;

LifeMarkers *LifeMarkers::create(const Size &screenSize)
{
    LifeMarkers *pRet = new(std::nothrow) LifeMarkers();
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
};

bool LifeMarkers::init(const Size &screenSize)
{
    if (!Node::init())
        return false;
    
    scheduleUpdate();
    
    lifeIndicatorSprite = Sprite::create("LifeMarker" + ulongToString(global_ShipSelect) + ".png");
    auto spriteSize = lifeIndicatorSprite->getContentSize();
    lifeIndicatorSprite->setPosition(32, screenSize.height - 32);
    lifeIndicatorSprite->setBlendFunc(BlendFunc::ALPHA_PREMULTIPLIED);
    lifeIndicatorSprite->setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("LifeMarkerProgram"));
    
    lifeText = Label::createWithSystemFont("100%", "fonts/Lato/Lato-Bold.ttf", 28);
    lifeText->setHorizontalAlignment(TextHAlignment::LEFT);
    
    addChild(lifeIndicatorSprite);
    addChild(lifeText);
    
    lifeText->setPosition(lifeIndicatorSprite->getPosition() + Vec2(22 + lifeText->getContentSize().width/2, 1));
    
    listener = _eventDispatcher->addCustomEventListener("LifeUpdate", CC_CALLBACK_1(LifeMarkers::lifeUpdate, this));
    
    targetLives = currentLives = 100;
    
    return true;
}

LifeMarkers::~LifeMarkers()
{
    _eventDispatcher->removeEventListener(listener);
}

void LifeMarkers::onEnterTransitionDidFinish()
{
    Node::onEnterTransitionDidFinish();
    recursiveResume(this);
}

void LifeMarkers::onEnter()
{
    Node::onEnter();
    recursivePause(this);
}

void LifeMarkers::onExitTransitionDidStart()
{
    Node::onExitTransitionDidStart();
    recursivePause(this);
}

void LifeMarkers::lifeUpdate(EventCustom *event)
{
    targetLives = std::abs(*static_cast<int*>(event->getUserData()));
}

void LifeMarkers::update(float delta)
{
    currentLives += 2 * delta * (targetLives - currentLives);
        
    if (std::abs(targetLives - currentLives) < 0.25) currentLives = targetLives;
    
    auto lastPos = lifeText->getPosition() - lifeText->getContentSize()/2;
    lifeText->setString(ulongToString((int)roundf(currentLives)) + "%");
    lifeText->setPosition(lastPos + lifeText->getContentSize()/2);
}

void LifeMarkers::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
    auto firstDraw = std::min(currentLives, targetLives)/MaxHealth;
    auto secondDraw = std::max(currentLives, targetLives)/MaxHealth;
    
    lifeIndicatorSprite->setColor(Color3B(255 * firstDraw, 255 * secondDraw, 0));
    
    Node::draw(renderer, transform, flags);
}
