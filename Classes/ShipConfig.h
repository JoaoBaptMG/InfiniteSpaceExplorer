//
//  ShipConfig.h
//  SpaceExplorer
//
//  Created by João Baptista on 28/06/15.
//
//

#ifndef __SpaceExplorer__ShipConfig__
#define __SpaceExplorer__ShipConfig__

#include <string>
#include <vector>
#include "cocos2d.h"

struct ShipConfig
{
    int pointsRequired;
    float damageMultiplier;
    std::vector<cocos2d::Vec2> jetPositions;
    float jetScale;
    
    bool collisionIsPolygon;
    float collisionRadius;
    cocos2d::Vec2 collisionOffset;
    std::vector<cocos2d::Vec2> collisionList;
};

const ShipConfig &getShipConfig(unsigned long index);
unsigned long getShipConfigSize();

#endif /* defined(__SpaceExplorer__ShipConfig__) */
