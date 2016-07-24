//
//  BackgroundNode.h
//  SpaceExplorer
//
//  Created by João Baptista on 03/07/15.
//
//

#ifndef __SpaceExplorer__BackgroundNode__
#define __SpaceExplorer__BackgroundNode__

#include "cocos2d.h"

class BackgroundNode : public cocos2d::Node
{
    void spawnBackground();
    float waitTime;
    
public:
    bool init() override;
    
    virtual void onEnterTransitionDidFinish() override;
    virtual void onExitTransitionDidStart() override;
    virtual void onEnter() override;
    
    virtual void update(float delta) override;
    
    CREATE_FUNC(BackgroundNode);
};

#endif /* defined(__SpaceExplorer__BackgroundNode__) */
