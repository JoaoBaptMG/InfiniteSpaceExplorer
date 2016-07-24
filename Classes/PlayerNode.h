//
//  PlayerNode.h
//  SpaceExplorer
//
//  Created by João Baptista on 02/03/15.
//
//

#ifndef __SpaceExplorer__PlayerNode__
#define __SpaceExplorer__PlayerNode__

#include "cocos2d.h"
#include "MotionProcessor.h"
#include "CollisionManager.h"

constexpr int MaxHealth = 100;

class PowerupIcon;

class PlayerNode : public cocos2d::Node
{
    // FIXME: Remove this
    cocos2d::Label *debugOrientationLabel;
    
    int health;
    float fixedUpdateInterval;
    bool damage, touching, alreadyPositioned, onShield, withShooter, invincible;
    MotionProcessor *motionProcessor;
    PowerupIcon *shieldIcon;
    
    cocos2d::Vector<cocos2d::Sprite*> jetFlames;
    float fixedAnimationInterval;
    int currentAnimation;
    
    float damageTimer;
    float tintAmount;
    float shooterTimer;
    
    void updateCollisionData();
    
    void takeDamage(const CollisionManager::HazardCollisionData &hazard);
    void takeShieldDamage(const CollisionManager::HazardCollisionData &hazard);
    
    void shootProjectile();
    bool projectileDamage(const CollisionManager::HazardCollisionData &hazard);
    
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    
    void createJetFlames();
    
    virtual void update(float delta) override;
    virtual void onEnterTransitionDidFinish() override;
    virtual void onExitTransitionDidStart() override;
    virtual void onEnter() override;
    
public:
    bool init();
    virtual ~PlayerNode();
    
    void increaseHealth(int health);
    void addShield();
    void changeShieldIcon(PowerupIcon *icon);
    void removeShield(bool exploding = false);
    void addShooter();
    void removeShooter();
    
    void turnInvincibility();
    
    CREATE_FUNC(PlayerNode);
};

#endif /* defined(__SpaceExplorer__PlayerNode__) */
