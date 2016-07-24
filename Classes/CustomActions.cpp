//
//  CustomActions.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 10/05/15.
//
//

#include "CustomActions.h"

using namespace cocos2d;

ExecFunc *ExecFunc::create(float duration, decltype(execFunc) func)
{
    ExecFunc *pRet = new(std::nothrow) ExecFunc();
    if (pRet && pRet->init(duration, func))
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

UpdateNode *UpdateNode::create(decltype(execFunc) func)
{
    UpdateNode *pRet = new(std::nothrow) UpdateNode();
    if (pRet && pRet->init(func))
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

RunOnChild *RunOnChild::create(std::string name, FiniteTimeAction *action)
{
    RunOnChild *pRet = new(std::nothrow) RunOnChild();
    if (pRet && pRet->init(name, action))
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

bool ExecFunc::init(float duration, decltype(execFunc) func)
{
    if (!ActionInterval::initWithDuration(duration))
        return false;
    
    execFunc = func;
    return true;
}

bool UpdateNode::init(decltype(execFunc) func)
{
    execFunc = func;
    return true;
}

bool RunOnChild::init(std::string name, FiniteTimeAction *action)
{
    this->action = action;
    action->retain();
    this->name = name;
    
    setDuration(action->getDuration());
    
    return true;
}

void ExecFunc::update(float interval)
{
    execFunc(getTarget(), interval);
}

void UpdateNode::step(float dt)
{
    execFunc(getTarget(), dt);
}

FiniteTimeAction *RunOnChild::reverse() const
{
    return RunOnChild::create(name, action->reverse());
}

void RunOnChild::startWithTarget(Node *node)
{
    auto actualTarget = node->getChildByName(name);
    if (actualTarget) action->startWithTarget(actualTarget);
    FiniteTimeAction::startWithTarget(node);
}

void RunOnChild::stop()
{
    if (action->getTarget() != nullptr) action->stop();
    FiniteTimeAction::stop();
}

RunOnChild::~RunOnChild()
{
    action->release();
}