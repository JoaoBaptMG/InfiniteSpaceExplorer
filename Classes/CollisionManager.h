//
//  CollisionManager.h
//  SpaceExplorer
//
//  Created by João Baptista on 05/03/15.
//
//

#ifndef __SpaceExplorer__CollisionManager__
#define __SpaceExplorer__CollisionManager__

#include "cocos2d.h"

class PlayerNode;

namespace CollisionManager
{
    enum class CollisionType : int
    {
        Circle, TwoOffsetCapsule, TwoNodeCapsule, Polygon
    };
    
    struct HazardCollisionData
    {
        struct HazardInfo
        {
            int damage;
            int projectileScore;
            bool deleteAnyway;
            bool penetratesShield;
            cocos2d::RefPtr<cocos2d::Node> companionNode[2];
        };
        
        CollisionType type;
        HazardInfo info;
        cocos2d::Node *positionNode = nullptr;
        cocos2d::Node *otherNode = nullptr;
        
        cocos2d::Vec2 offset[2];
        float radius;
        
        const cocos2d::Vec2* polygonList = nullptr;
        int polygonListSize; bool copyList = false;
        
    private:
        HazardCollisionData() : otherNode(nullptr), polygonList(nullptr), copyList(false) {}
        
    public:
        HazardCollisionData(HazardCollisionData &&other)
        : type(other.type), positionNode(other.positionNode), otherNode(other.otherNode), offset{other.offset[0], other.offset[1]},
        radius(other.radius), polygonList(other.polygonList), polygonListSize(other.polygonListSize), info(other.info), copyList(other.copyList)
        {
            other.positionNode = nullptr;
            other.otherNode = nullptr;
            
            if (copyList)
            {
                other.polygonList = nullptr;
                other.copyList = false;
            }
        }
        
        inline static HazardCollisionData createCircle(cocos2d::Node *positionNode, cocos2d::Vec2 offset, float radius, const HazardInfo &info)
        {
            HazardCollisionData data;
            data.type = CollisionType::Circle;
            data.positionNode = positionNode;
            data.positionNode->retain();
            data.offset[0] = offset;
            data.radius = radius;
            data.info = info;
            
            data.copyList = false;
            
            return std::move(data);
        }
        
        inline static HazardCollisionData createTwoOffsetCapsule(cocos2d::Node *positionNode, cocos2d::Vec2 offset1, cocos2d::Vec2 offset2, float radius, const HazardInfo &info)
        {
            HazardCollisionData data;
            data.type = CollisionType::TwoOffsetCapsule;
            data.positionNode = positionNode;
            data.positionNode->retain();
            data.offset[0] = offset1;
            data.offset[1] = offset2;
            data.radius = radius;
            data.info = info;
            
            data.copyList = false;
            
            return data;
        }
        
        inline static HazardCollisionData createTwoNodeCapsule(cocos2d::Node *positionNode, cocos2d::Node *otherNode, cocos2d::Vec2 offset, float radius, const HazardInfo &info)
        {
            HazardCollisionData data;
            data.type = CollisionType::TwoNodeCapsule;
            data.positionNode = positionNode;
            data.positionNode->retain();
            data.otherNode = otherNode;
            data.otherNode->retain();
            data.offset[0] = offset;
            data.radius = radius;
            data.info = info;
            
            data.copyList = false;
            
            return data;
        }
        
        inline static HazardCollisionData createPolygon(cocos2d::Node *positionNode, cocos2d::Vec2 offset, const cocos2d::Vec2* polygonList, int polygonListSize, const HazardInfo &info, bool copyList = false)
        {
            HazardCollisionData data;
            data.type = CollisionType::Polygon;
            data.positionNode = positionNode;
            data.positionNode->retain();
            data.offset[0] = offset;
            data.info = info;
            
            data.polygonListSize = polygonListSize;
            if ((data.copyList = copyList))
            {
                cocos2d::Vec2 *newList = new cocos2d::Vec2[polygonListSize];
                for (int i = 0; i < polygonListSize; i++) newList[i] = polygonList[i];
                data.polygonList = newList;
            }
            else data.polygonList = polygonList;
            
            return data;
        }
        
        ~HazardCollisionData()
        {
            if (positionNode) positionNode->release();
            if (otherNode) otherNode->release();
            if (copyList) delete[] polygonList;
        }
    };
    
    struct PlayerCollisionData
    {
        cocos2d::Node *positionNode;
        
        CollisionType type;
        
        cocos2d::Vec2 offset;
        float radius;
        
        const cocos2d::Vec2* polygonList;
        int polygonListSize;
        
        std::function<void(const HazardCollisionData&)> delegate;
        std::function<bool(const HazardCollisionData&)> projectileDelegate;
        
        inline PlayerCollisionData(cocos2d::Node *positionNode, cocos2d::Vec2 offset, float radius, decltype(delegate) delegate, decltype(projectileDelegate) projectileDelegate)
        : positionNode(positionNode), offset(offset), radius(radius), delegate(delegate), projectileDelegate(projectileDelegate),
          polygonList(nullptr), polygonListSize(0), type(CollisionType::Circle)
        {
            this->positionNode->retain();
        }
        
        inline PlayerCollisionData(cocos2d::Node *positionNode, const cocos2d::Vec2* polygonList, int polygonListSize, float polygonListScale, decltype(delegate) delegate, decltype(projectileDelegate) projectileDelegate)
        : positionNode(positionNode), polygonList(nullptr), polygonListSize(polygonListSize), delegate(delegate), projectileDelegate(projectileDelegate),
        offset(0, 0), radius(0), type(CollisionType::Polygon)
        {
            this->positionNode->retain();
            
            cocos2d::Vec2 *tempList = new cocos2d::Vec2[polygonListSize];
            for (int i = 0; i < polygonListSize; i++) tempList[i] = polygonList[i] * polygonListScale;
            this->polygonList = tempList;
        }
        
        inline PlayerCollisionData &operator=(PlayerCollisionData&& other)
        {
            if (positionNode) positionNode->release();
            if (type == CollisionType::Polygon)
            {
                delete[] polygonList;
                polygonList = nullptr;
            }
            
            positionNode = other.positionNode;
            type = other.type;
            delegate = other.delegate;
            projectileDelegate = other.projectileDelegate;
            
            other.positionNode = nullptr;
            if (other.type == CollisionType::Polygon)
            {
                polygonList = other.polygonList;
                other.polygonList = nullptr;
                polygonListSize = other.polygonListSize;
            }
            else
            {
                offset = other.offset;
                radius = other.radius;
            }
            
            return *this;
        }
        
        PlayerCollisionData() {}
        
        ~PlayerCollisionData()
        {
            if (positionNode) positionNode->release();
            if (type == CollisionType::Polygon) delete[] polygonList;
        }
    };
    
    struct PowerupCollisionData
    {
        cocos2d::Node *positionNode;
        cocos2d::Vec2 offset;
        float radius;
        
        std::function<void(const PlayerCollisionData&)> delegate;
        inline PowerupCollisionData(cocos2d::Node *positionNode, cocos2d::Vec2 offset, float radius, decltype(delegate) delegate)
        : positionNode(positionNode), offset(offset), radius(radius), delegate(delegate)
        {
            if (positionNode) this->positionNode->retain();
        }
        
        inline PowerupCollisionData(PowerupCollisionData &&other)
        : positionNode(other.positionNode), offset(other.offset), radius(other.radius), delegate(other.delegate)
        {
            other.positionNode = nullptr;
        }
        
        ~PowerupCollisionData()
        {
            if (positionNode) positionNode->release();
        }
    };
    
    struct ProjectileCollisionData
    {
        cocos2d::Node *positionNode;
        cocos2d::Vec2 offset;
        float radius;
        
        inline ProjectileCollisionData(cocos2d::Node *positionNode, cocos2d::Vec2 offset, float radius) : positionNode(positionNode), offset(offset), radius(radius)
        {
            if (positionNode) this->positionNode->retain();
        }
        
        inline ProjectileCollisionData(ProjectileCollisionData &&other)
        : positionNode(other.positionNode), offset(other.offset), radius(other.radius)
        {
            other.positionNode = nullptr;
        }
        
        ~ProjectileCollisionData()
        {
            if (positionNode) positionNode->release();
        }
    };
    
    void setPlayer(PlayerCollisionData &&collisionData);
    void addHazard(HazardCollisionData &&collisionData);
    void addPowerup(PowerupCollisionData &&collisionData);
    void addProjectile(ProjectileCollisionData &&collisionData);
    
    void update();
    
    void clearCollisionData();
}

#endif /* defined(__SpaceExplorer__CollisionManager__) */
