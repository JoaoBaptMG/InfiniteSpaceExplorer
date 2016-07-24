//
//  BackgroundNode.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 03/07/15.
//
//

#include "BackgroundNode.h"
#include "Defaults.h"

extern float global_AdvanceSpeed;

using namespace cocos2d;

bool BackgroundNode::init()
{
    if (!Node::init())
        return false;
    
    waitTime = 8;
    scheduleUpdate();
    
    return true;
}

void BackgroundNode::onEnterTransitionDidFinish()
{
    Node::onEnterTransitionDidFinish();
    recursiveResume(this);
}

void BackgroundNode::onExitTransitionDidStart()
{
    Node::onExitTransitionDidStart();
    recursivePause(this);
}

void BackgroundNode::onEnter()
{
    Node::onEnter();
    recursivePause(this);
}

constexpr float MoveSpeedMin = 4, MoveSpeedMax = 12;

static const Color3B backgroundColors[] = { 0xFFCC33_c3, 0xFF6666_c3, 0x999933_c3, 0xFF66CC_c3, 0x996666_c3 };
constexpr int backgroundColorsSize = sizeof(backgroundColors)/sizeof(backgroundColors[0]);

void BackgroundNode::spawnBackground()
{
    const Size& size = getScene()->getContentSize();
    
    int depth = random(1, 8);
    auto radius = random(0.8, 3.6) * size.height;
    
    auto node = Sprite::create("common/Background.png");
    if (!node->getTexture()->hasMipmaps()) node->getTexture()->generateMipmap();
    node->setPosition(size.width + radius, size.height/2 + random(-.75*radius, .75*radius));
    node->setScale(2*radius/node->getContentSize().height);
    node->setColor(backgroundColors[random(0, backgroundColorsSize-1)]);
    node->setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("BackgroundProgram"));
    node->setBlendFunc(BlendFunc::ALPHA_PREMULTIPLIED);
    addChild(node, depth - 18);
    
    float speed = 8 * random(MoveSpeedMin, MoveSpeedMax) / (9 - depth);
    float distance = size.width + 2*radius;
    node->runAction(Sequence::createWithTwoActions(MoveBy::create(distance/speed, Vec2(-distance, 0)), RemoveSelf::create()));
    
    waitTime += random(12.0f, 144.0f)/depth;
}

void BackgroundNode::update(float delta)
{
    Node::update(delta);
    
    waitTime -= delta * (1 + global_AdvanceSpeed/150);
    if (waitTime < 0) spawnBackground();
    
    if (global_AdvanceSpeed > 0)
        for (Node *node : getChildren())
            node->setPositionX(node->getPositionX() - 0.5 * delta * global_AdvanceSpeed / (9 - node->getLocalZOrder()));
}