//
//  CustomActions.h
//  SpaceExplorer
//
//  Created by João Baptista on 10/05/15.
//
//

#ifndef __SpaceExplorer__CustomActions__
#define __SpaceExplorer__CustomActions__

#include "cocos2d.h"

class ExecFunc : public cocos2d::ActionInterval
{
    std::function<void(cocos2d::Node*,float)> execFunc;
    
    bool init(float duration, decltype(execFunc) func);
    
public:
    static ExecFunc *create(float duration, decltype(execFunc) func);
    virtual void update(float interval) override;
};

class UpdateNode : public cocos2d::Action
{
    std::function<void(cocos2d::Node*,float)> execFunc;
    
    bool init(decltype(execFunc) func);
    
public:
    static UpdateNode *create(decltype(execFunc) func);
    virtual void step(float dt) override;
    virtual bool isDone() const override { return false; }
};

class RunOnChild : public cocos2d::FiniteTimeAction
{
    cocos2d::FiniteTimeAction *action;
    std::string name;
    
    bool init(std::string name, cocos2d::FiniteTimeAction* action);
    
public:
    static RunOnChild *create(std::string name, cocos2d::FiniteTimeAction* action);
    
    virtual cocos2d::FiniteTimeAction *reverse() const override;
    virtual void startWithTarget(cocos2d::Node *node) override;
    virtual void step(float dt) override { action->step(dt); }
    virtual void update(float time) override { action->update(time); }
    virtual bool isDone() const override { return action->isDone(); }
    virtual void stop() override;
    
    virtual ~RunOnChild();
};

#endif /* defined(__SpaceExplorer__CustomActions__) */
