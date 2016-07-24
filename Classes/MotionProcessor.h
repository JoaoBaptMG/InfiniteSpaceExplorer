//
//  MotionProcessor.h
//  SpaceExplorer
//
//  Created by João Baptista on 06/03/15.
//
//

#ifndef __SpaceExplorer__MotionProcessor__
#define __SpaceExplorer__MotionProcessor__

#include "cocos2d.h"

class MotionProcessor
{
public:
    virtual ~MotionProcessor() {}
    virtual void calibrate() = 0;
    virtual cocos2d::Vec2 getDirectionVector() = 0;
    
    virtual int getScreenRotation() = 0;
    
protected:
    MotionProcessor() {}
    
    friend MotionProcessor *createMotionProcessor();
};

MotionProcessor *createMotionProcessor();

#endif /* defined(__SpaceExplorer__MotionProcessor__) */
