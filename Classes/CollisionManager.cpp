//
//  CollisionManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 05/03/15.
//
//

#include "CollisionManager.h"

using namespace cocos2d;

namespace CollisionManager
{
    static PlayerCollisionData playerCollision;
    static std::list<HazardCollisionData> hazardCollisions;
    static std::list<PowerupCollisionData> powerupCollisions;
    static std::list<ProjectileCollisionData> projectileCollisions;

    void setPlayer(PlayerCollisionData &&collisionData)
    {
        playerCollision = std::move(collisionData);
    }

    void addHazard(HazardCollisionData &&collisionData)
    {
        hazardCollisions.push_back(std::move(collisionData));
    }

    void addPowerup(PowerupCollisionData &&collisionData)
    {
        powerupCollisions.push_back(std::move(collisionData));
    }
    
    void addProjectile(ProjectileCollisionData &&collisionData)
    {
        projectileCollisions.push_back(std::move(collisionData));
    }

    bool checkHazardCollisionCircle(const Vec2 &player, float playerRadius, const HazardCollisionData &hazard);
    bool checkHazardCollisionPolygon(const Vec2 *playerList, int playerListSize, const HazardCollisionData &hazard);

    inline bool circleIntersection(const Vec2 &shape1, float shape1radius, const Vec2 &shape2, float shape2radius)
    {
        auto radius = shape1radius + shape2radius;
        return (shape1 - shape2).lengthSquared() <= radius*radius;
    }

    inline bool capsuleCircleIntersection(const Vec2 &shape, float shapeRadius, const Vec2 &line1, const Vec2 &line2, float lineRadius)
    {
        auto radius = shapeRadius + lineRadius;
        
        // Projection test
        float p1 = 0;
        float p2 = (line2 - line1).lengthSquared();
        float p = (shape - line1).dot(line2 - line1);
        
        // Test endpoints
        if (p < p1) return (shape - line1).lengthSquared() < radius*radius;
        if (p > p2) return (shape - line2).lengthSquared() < radius*radius;
        
        // Distance test
        auto pt = line1 + (line2 - line1) * p/p2;
        return (pt - shape).lengthSquared() <= radius*radius;
    }
    
    inline bool capsuleCapsuleIntersection(const Vec2 &line1_1, const Vec2 &line1_2, float line1Radius, const Vec2 &line2_1, const Vec2 &line2_2, float line2Radius)
    {
        // Try segment/segment first
        if ((line2_1 - line1_1).cross(line1_2 - line1_1) * (line2_2 - line1_1).cross(line1_2 - line1_1) <= 0 &&
            (line1_1 - line2_1).cross(line2_2 - line2_1) * (line1_2 - line2_1).cross(line2_2 - line1_1) <= 0) return true;
        
        // We'll use the previous algorithm, for improvement
        if (capsuleCircleIntersection(line1_1, line1Radius, line2_1, line2_2, line2Radius)) return true;
        if (capsuleCircleIntersection(line1_2, line1Radius, line2_1, line2_2, line2Radius)) return true;
        if (capsuleCircleIntersection(line2_1, line2Radius, line1_1, line1_2, line1Radius)) return true;
        if (capsuleCircleIntersection(line2_2, line2Radius, line1_1, line1_2, line1Radius)) return true;
        
        return false;
    }

    inline bool polygonCircleIntersection(const Vec2 &shape, float shapeRadius, const Vec2 *pointList, int pointSize)
    {
        // Sanity check
        if (pointSize < 3)
        {
            CCLOG("Point list has less than 3 elements!");
            return false;
        }
        
        bool insidePolygon = false;
        
        // We will simultaneoulsy conduct two tests
        // Firstly, we have the "shell" test: just test intersection with the sides
        // Second, we will test if the center is inside the polygon
        for (int i = 0; i < pointSize; i++)
        {
            Vec2 point1 = pointList[i];
            Vec2 point2 = pointList[(i+1)%pointSize];
            
            // First test
            if (point1 == point2) continue;
            if (capsuleCircleIntersection(shape, shapeRadius, point1, point2, 0)) return true;
            
            // Second test
            if (point1.x < shape.x && point2.x < shape.x) continue;
            if (point1.x < shape.x) point1 = point2 + (shape.x - point2.x) / (point1.x - point2.x) * (point1 - point2);
            if (point2.x < shape.x) point2 = point1 + (shape.x - point1.x) / (point2.x - point1.x) * (point2 - point1);
            
            if ((point1.y >= shape.y) != (point2.y >= shape.y)) insidePolygon = !insidePolygon;
        }
        
        // Check if the second test is true
        return insidePolygon;
    }
    
    inline bool polygonCapsuleIntersection(const Vec2 &line1, const Vec2 &line2, float lineRadius, const Vec2 *pointList, int pointSize)
    {
        // Sanity check
        if (pointSize < 3)
        {
            CCLOG("Point list has less than 3 elements!");
            return false;
        }
        
        bool insidePolygon1 = false, insidePolygon2 = false;
        
        // We will simultaneoulsy conduct two tests
        // Firstly, we have the "shell" test: just test intersection with the sides
        // Second, we will test if the any of the endpoints are inside the polygon
        for (int i = 0; i < pointSize; i++)
        {
            auto point1 = pointList[i];
            auto point2 = pointList[(i+1)%pointSize];
            
            // First test
            if (point1 == point2) continue;
            if (capsuleCapsuleIntersection(line1, line2, lineRadius, point1, point2, 0)) return true;
            
            // Second test
            Vec2 point1c = point1, point2c = point2;
            
            if (point1.x < line1.x && point2.x < line1.x) continue;
            if (point1.x < line1.x) point1 = point2 + (line1.x - point2.x) / (point1.x - point2.x) * (point1 - point2);
            if (point2.x < line1.x) point2 = point1 + (line1.x - point1.x) / (point2.x - point1.x) * (point2 - point1);
            if ((point1.y >= line1.y) != (point2.y >= line1.y)) insidePolygon1 = !insidePolygon1;
            
            if (point1c.x < line2.x && point2c.x < line2.x) continue;
            if (point1c.x < line2.x) point1c = point2c + (line2.x - point2c.x) / (point1c.x - point2c.x) * (point1c - point2c);
            if (point2c.x < line2.x) point2c = point1c + (line2.x - point1c.x) / (point2c.x - point1c.x) * (point2c - point1c);
            if ((point1c.y >= line2.y) != (point2c.y >= line2.y)) insidePolygon2 = !insidePolygon2;
        }
        
        // Check if the second test is true
        return insidePolygon1 || insidePolygon2;
    }
    
    inline bool polygonPolygonIntersection(const Vec2 *pointList1, int pointSize1, const Vec2 *pointList2, int pointSize2)
    {
        // Sanity check
        if (pointSize1 < 3)
        {
            CCLOG("Point list (1) has less than 3 elements!");
            return false;
        }
        if (pointSize2 < 3)
        {
            CCLOG("Point list (2) has less than 3 elements!");
            return false;
        }
        
        // Iterate through each segment of each polygon
        for (int i = 0; i < pointSize1; i++)
        {
            for (int j = 0; j < pointSize2; j++)
            {
                auto point1_1 = pointList1[i];
                auto point1_2 = pointList1[(i+1)%pointSize1];
                auto point2_1 = pointList2[j];
                auto point2_2 = pointList2[(j+1)%pointSize2];
                
                if (point1_1 == point1_2 || point2_1 == point2_2) continue;
                
                // segment/segment intersection, better than capsule/segment intersection
                if ((point2_1 - point1_1).cross(point1_2 - point1_1) * (point2_2 - point1_1).cross(point1_2 - point1_1) <= 0 &&
                    (point1_1 - point2_1).cross(point2_2 - point2_1) * (point1_2 - point2_1).cross(point2_2 - point1_1) <= 0) return true;
            }
        }
        
        // Nothing... ?
        return false;
    }
    
    Vec2 tempPlayerTransformStore[40], tempHazardTransformStore[32];
    
    inline float combinedScale(Node* node)
    {
        float scale = 1;
        while (node)
        {
            scale *= node->getScale();
            node = node->getParent();
        }
        return scale;
    }
    
    void update()
    {
        if (!playerCollision.positionNode) return;
        
        if (playerCollision.positionNode->getParent() == nullptr)
        {
            playerCollision.positionNode->release();
            playerCollision.positionNode = nullptr;
            return;
        }
        
        for (auto it = projectileCollisions.cbegin(); it != projectileCollisions.cend();)
            if (it->positionNode->getParent() == nullptr) it = projectileCollisions.erase(it);
            else ++it;
        
        auto player = playerCollision.positionNode->convertToWorldSpaceAR(playerCollision.offset);
        auto playerRadius = playerCollision.radius * combinedScale(playerCollision.positionNode);
        
        if (playerCollision.type == CollisionType::Polygon)
            for (int i = 0; i < playerCollision.polygonListSize; i++)
                tempPlayerTransformStore[i] = playerCollision.positionNode->convertToWorldSpaceAR(playerCollision.polygonList[i]);
        
        for (auto it = hazardCollisions.cbegin(); it != hazardCollisions.cend();)
            if (it->positionNode->getParent() == nullptr ||
                (it->type == CollisionType::TwoNodeCapsule && it->otherNode->getParent() == nullptr))
                it = hazardCollisions.erase(it);
            else
            {
                for (auto it2 = projectileCollisions.cbegin(); it2 != projectileCollisions.end();)
                {
                    auto projectile = it2->positionNode->convertToWorldSpaceAR(it2->offset);
                    auto projectileRadius = it2->radius * combinedScale(it2->positionNode);
                    
                    if (checkHazardCollisionCircle(projectile, projectileRadius, *it))
                    {
                        // Custom optimization for projectiles only!
                        if (playerCollision.projectileDelegate(*it))
                        {
                            it2->positionNode->removeFromParent();
                            it2 = projectileCollisions.erase(it2);
                        
                            // The hazard should have been destroyed, so another good optimization
                            goto outerContinue;
                        }
                        else ++it2;
                    }
                    else ++it2;
                }
                
                // In order to the 'outerContinue' to function
                {
                    bool intersection = playerCollision.type == CollisionType::Polygon ?
                        checkHazardCollisionPolygon(tempPlayerTransformStore, playerCollision.polygonListSize, *it) :
                        checkHazardCollisionCircle(player, playerRadius, *it);
                    
                    if (intersection) playerCollision.delegate(*it);
                }

            outerContinue: ++it;
            }
        
        for (auto it = powerupCollisions.cbegin(); it != powerupCollisions.cend();)
            if (it->positionNode->getParent() == nullptr)
                it = powerupCollisions.erase(it);
            else
            {
                auto powerup = it->positionNode->convertToWorldSpaceAR(it->offset);
                auto powerupRadius = it->radius * combinedScale(it->positionNode);
                
                bool intersection = playerCollision.type == CollisionType::Polygon ?
                    polygonCircleIntersection(powerup, powerupRadius, tempPlayerTransformStore, playerCollision.polygonListSize) :
                    circleIntersection(player, playerRadius, powerup, powerupRadius);
                
                if (intersection)
                {
                    // Custom optimization for powerups only!
                    it->delegate(playerCollision);
                    it = powerupCollisions.erase(it);
                }
                else ++it;
            }
    }

    bool checkHazardCollisionCircle(const Vec2 &player, float playerRadius, const HazardCollisionData &hazardData)
    {
        switch (hazardData.type)
        {
            case CollisionType::Circle:
            {
                auto hazard = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto hazardRadius = hazardData.radius * combinedScale(hazardData.positionNode);
                
                return circleIntersection(player, playerRadius, hazard, hazardRadius);
            }
            case CollisionType::TwoOffsetCapsule:
            {
                auto hazard1 = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto hazard2 = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[1]);
                auto hazardRadius = hazardData.radius * combinedScale(hazardData.positionNode);
                
                return capsuleCircleIntersection(player, playerRadius, hazard1, hazard2, hazardRadius);
            }
            case CollisionType::TwoNodeCapsule:
            {
                auto hazard1 = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto hazard2 = hazardData.otherNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto minScale = std::min(combinedScale(hazardData.positionNode), combinedScale(hazardData.otherNode));
                auto hazardRadius = hazardData.radius * minScale;
                
                return capsuleCircleIntersection(player, playerRadius, hazard1, hazard2, hazardRadius);
            }
            case CollisionType::Polygon:
            {
                auto playerLocal = hazardData.positionNode->convertToNodeSpaceAR(player);
                auto playerLocalRadius = playerRadius / combinedScale(hazardData.positionNode);
                
                return polygonCircleIntersection(playerLocal, playerLocalRadius, hazardData.polygonList, hazardData.polygonListSize);
            }
        }
    }
    
    bool checkHazardCollisionPolygon(const Vec2 *playerList, int playerListSize, const HazardCollisionData &hazardData)
    {
        switch (hazardData.type)
        {
            case CollisionType::Circle:
            {
                auto hazard = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto hazardRadius = hazardData.radius * combinedScale(hazardData.positionNode);
                
                return polygonCircleIntersection(hazard, hazardRadius, playerList, playerListSize);
            }
            case CollisionType::TwoOffsetCapsule:
            {
                auto hazard1 = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto hazard2 = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[1]);
                auto hazardRadius = hazardData.radius * combinedScale(hazardData.positionNode);
                
                return polygonCapsuleIntersection(hazard1, hazard2, hazardRadius, playerList, playerListSize);
            }
            case CollisionType::TwoNodeCapsule:
            {
                auto hazard1 = hazardData.positionNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto hazard2 = hazardData.otherNode->convertToWorldSpaceAR(hazardData.offset[0]);
                auto minScale = std::min(combinedScale(hazardData.positionNode), combinedScale(hazardData.otherNode));
                auto hazardRadius = hazardData.radius * minScale;
                
                return polygonCapsuleIntersection(hazard1, hazard2, hazardRadius, playerList, playerListSize);
            }
            case CollisionType::Polygon:
            {
                for (int i = 0; i < hazardData.polygonListSize; i++)
                    tempHazardTransformStore[i] = hazardData.positionNode->convertToWorldSpaceAR(hazardData.polygonList[i]);
                
                return polygonPolygonIntersection(playerList, playerListSize, tempHazardTransformStore, hazardData.polygonListSize);
            }
        }
    }
    
    void clearCollisionData()
    {
        hazardCollisions.clear();
        powerupCollisions.clear();
        projectileCollisions.clear();
    }
}