//
//  ShipConfig.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 28/06/15.
//
//

#include "ShipConfig.h"

using namespace cocos2d;

static std::vector<ShipConfig> configs;
static bool init = false;

void parseConfigValueVector(const ValueVector &configValueVector, std::vector<ShipConfig> &configs)
{
    for (const Value& value : configValueVector)
    {
        const ValueMap& map = value.asValueMap();
        
        ShipConfig cfg;
        cfg.pointsRequired = map.at("PointsRequired").asInt();
        cfg.jetScale = map.at("JetScale").asFloat();
        cfg.damageMultiplier = map.at("DamageMultiplier").asFloat();
        
        for (const Value &posValue : map.at("JetPositions").asValueVector())
            cfg.jetPositions.push_back(PointFromString(posValue.asString()));
        
        auto polygonListIt = map.find("CollisionPolygon");
        if ((cfg.collisionIsPolygon = polygonListIt != map.end()))
        {
            for (const Value &colValue : polygonListIt->second.asValueVector())
                cfg.collisionList.push_back(PointFromString(colValue.asString()));
        }
        else
        {
            cfg.collisionRadius = map.at("CollisionRadius").asFloat();
            cfg.collisionOffset = PointFromString(map.at("CollisionOffset").asString());
        }
        
        configs.push_back(std::move(cfg));
    }
}

const ShipConfig &getShipConfig(unsigned long index)
{
    if (!init)
    {
        ValueVector parseFile = FileUtils::getInstance()->getValueVectorFromFile("ShipConfig.plist");
        parseConfigValueVector(parseFile, configs);
        init = true;
    }
    
    return configs[index];
}

unsigned long getShipConfigSize()
{
    if (!init) getShipConfig(0); // trigger config creation
    return configs.size();
}