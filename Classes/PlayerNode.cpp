//
//  PlayerNode.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 02/03/15.
//
//

#include "PlayerNode.h"
#include "CollisionManager.h"
#include "Defaults.h"
#include "PowerupSpawner.h"
#include "CustomActions.h"
#include "ShipConfig.h"
#include "GameScene.h"
#include "SoundManager.h"

unsigned long global_ShipSelect = 0;

constexpr float BlinkTime = 0.0625f;

constexpr int SHIELD_BORDER = 1;
constexpr int SHIELD_BACK = 2;
constexpr int SHIELD_FRONT = 3;

constexpr int SHOOT_DRONE = 4;

constexpr int FRONT_SPRITE = 9;
constexpr int BACK_SPRITE = 10;

constexpr float AnimationRate = 30.0f;
constexpr float PlayerScale = 0.78125f;

using namespace cocos2d;

void PlayerNode::updateCollisionData()
{
    const auto& config = getShipConfig(global_ShipSelect);
    if (config.collisionIsPolygon)
        CollisionManager::setPlayer(CollisionManager::PlayerCollisionData(this, &config.collisionList[0], (int)config.collisionList.size(), PlayerScale,
                                                                          CC_CALLBACK_1(PlayerNode::takeDamage, this), CC_CALLBACK_1(PlayerNode::projectileDamage, this)));
    else CollisionManager::setPlayer(CollisionManager::PlayerCollisionData(this, config.collisionOffset * PlayerScale, config.collisionRadius * PlayerScale,
                                                                              CC_CALLBACK_1(PlayerNode::takeDamage, this), CC_CALLBACK_1(PlayerNode::projectileDamage, this)));
}

bool PlayerNode::init()
{
    if (!Node::init())
        return false;
    
    scheduleUpdate();
    
    fixedUpdateInterval = 0;
    shieldIcon = nullptr;
    motionProcessor = nullptr;
    
    damage = touching = alreadyPositioned = onShield = withShooter = invincible = false;
    health = MaxHealth;
    
    damageTimer = 2*BlinkTime;
    
    updateCollisionData();
    
    auto listener = EventListenerTouchOneByOne::create();
    listener->onTouchBegan = CC_CALLBACK_2(PlayerNode::onTouchBegan, this);
    listener->onTouchEnded = CC_CALLBACK_2(PlayerNode::onTouchEnded, this);
    listener->onTouchCancelled = listener->onTouchEnded;
    listener->setSwallowTouches(true);
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    setName("PlayerNode");
    setCascadeOpacityEnabled(true);
    
    tintAmount = 0;
    
    createJetFlames();
    
    auto sprite = Sprite::createWithSpriteFrameName("PlayerShape" + ulongToString(global_ShipSelect) + ".png");
    sprite->setScale(0.5 * PlayerScale);
    addChild(sprite, 1, FRONT_SPRITE);
    
    sprite->setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("PlayerShapeProgram"));
    sprite->getGLProgramState()->setUniformVec3("tintColor", { 1, 1, 1 });
    sprite->getGLProgramState()->setUniformCallback("tintAmount", [=] (GLProgram *program, Uniform *uniform)
    {
        program->setUniformLocationWith1f(uniform->location, tintAmount);
    });
    
    auto spriteFrame = SpriteFrameCache::getInstance()->getSpriteFrameByName("PlayerShape" + ulongToString(global_ShipSelect) + "b.png");
    if (spriteFrame)
    {
        auto backSprite = Sprite::createWithSpriteFrame(spriteFrame);
        backSprite->setPosition(getContentSize()/2);
        backSprite->setScale(0.5 * PlayerScale);
        backSprite->setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("PlayerShapeProgram"));
        addChild(backSprite, -2, BACK_SPRITE);
        
        backSprite->getTexture()->setAntiAliasTexParameters();
    }
    
    sprite->getTexture()->setAntiAliasTexParameters();
    
    return true;
}

PlayerNode::~PlayerNode()
{
    GLProgramState::getOrCreateWithGLProgramName("PlayerShapeProgram")->setUniformCallback("tintAmount", nullptr);
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    if (debugOrientationLabel->getParent() != nullptr) debugOrientationLabel->removeFromParent();
    debugOrientationLabel->release();
#endif
}

void PlayerNode::createJetFlames()
{
    for (Vec2 pos : getShipConfig(global_ShipSelect).jetPositions)
    {
        auto jet = Sprite::createWithSpriteFrameName("JetFire1.png");
        jet->setPosition(Vec2(pos.x - 32, pos.y - 32) * PlayerScale);
        jet->setScale(getShipConfig(global_ShipSelect).jetScale * PlayerScale);
        addChild(jet, -1);
        
        jet->setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("PlayerShapeProgram"));
        jet->getTexture()->setAntiAliasTexParameters();
        
        jetFlames.pushBack(jet);
    }
    
    fixedAnimationInterval = 0.0f;
    currentAnimation = 0;
}

void PlayerNode::takeDamage(const CollisionManager::HazardCollisionData &hazard)
{
    bool canHit = !damage && !invincible;
    
    if (canHit || hazard.info.deleteAnyway)
    {
        hazard.positionNode->removeFromParentAndCleanup(true);
        if (hazard.otherNode) hazard.otherNode->removeFromParentAndCleanup(true);
        if (hazard.info.companionNode[0]) hazard.info.companionNode[0]->removeFromParent();
        if (hazard.info.companionNode[1]) hazard.info.companionNode[1]->removeFromParent();
    }
    
    if (canHit)
    {
        SoundManager::play("common/Damage.wav");
        
        damage = true;
        health = std::max(health - int(hazard.info.damage * getShipConfig(global_ShipSelect).damageMultiplier), 0);
        
        _eventDispatcher->dispatchCustomEvent("LifeUpdate", &health);
        runAction(Sequence::create(DelayTime::create(1.0), CallFunc::create([this] { damage = false; }), nullptr));
    }
}

void PlayerNode::takeShieldDamage(const CollisionManager::HazardCollisionData &hazard)
{
    if (!invincible)
    {
        hazard.positionNode->removeFromParentAndCleanup(true);
        if (hazard.otherNode) hazard.otherNode->removeFromParentAndCleanup(true);
        if (hazard.info.companionNode[0]) hazard.info.companionNode[0]->removeFromParent();
        if (hazard.info.companionNode[1]) hazard.info.companionNode[1]->removeFromParent();
        
        if (hazard.info.penetratesShield)
        {
            damage = true;
            shieldIcon->collapse(true);
            removeShield(true);
            runAction(Sequence::create(DelayTime::create(1.0), CallFunc::create([this] { damage = false; }), nullptr));
        }
        else
        {
            for (int i : { SHIELD_BORDER, SHIELD_BACK, SHIELD_FRONT })
            {
                getChildByTag(i)->setOpacity(100);
                getChildByTag(i)->runAction(FadeTo::create(0.5, 255));
            }
        }
    }
}

bool PlayerNode::projectileDamage(const CollisionManager::HazardCollisionData &hazard)
{
    if (hazard.info.projectileScore != -1)
    {
        int val = hazard.info.projectileScore;
        _eventDispatcher->dispatchCustomEvent("ScoreUpdate", &val);
        
        for (Node *hz : { hazard.positionNode, hazard.otherNode, hazard.info.companionNode[0].get(), hazard.info.companionNode[1].get() })
        {
            if (!hz) continue;
            hz->stopAllActions();
            hz->removeFromParent();
        }
        
        SoundManager::play("common/FireballSplit.wav");
        
        return true;
    }
    else return false;
}

bool PlayerNode::onTouchBegan(Touch *touch, Event *event)
{
    auto size = getScene()->getContentSize();
    return touching = Rect(0, 0, size.width/2, size.height).containsPoint(touch->getLocation());
}

void PlayerNode::onTouchEnded(Touch *touch, Event *event)
{
    if (touching)
    {
        touching = false;
        motionProcessor->calibrate();
    }
}

void PlayerNode::onEnter()
{
    Node::onEnter();
    recursivePause(this);
    auto size = static_cast<GameScene*>(getParent()->getParent())->getPlayfieldSize();
    
    if (!alreadyPositioned)
    {
        setPosition(48, size.height/2);
        alreadyPositioned = true;
        
        int opHealth = -health;
        _eventDispatcher->dispatchCustomEvent("LifeUpdate", &opHealth);
        
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        getParent()->getParent()->addChild(debugOrientationLabel);
#endif
    }
}

void PlayerNode::onEnterTransitionDidFinish()
{
    Node::onEnterTransitionDidFinish();
    recursiveResume(this);
    touching = false;
    
    motionProcessor = createMotionProcessor();
}

void PlayerNode::onExitTransitionDidStart()
{
    Node::onExitTransitionDidStart();
    recursivePause(this);
    touching = true;
    
    delete motionProcessor;
    motionProcessor = nullptr;
}

void PlayerNode::increaseHealth(int amount)
{
    if (health < MaxHealth)
    {
        tintAmount = 1;
        runAction(ExecFunc::create(1.0, [=] (Node *node, float time) { tintAmount = 1-time; }));
    }
    
    health = std::min(health + amount, MaxHealth);
    
    int opHealth = -health;
    _eventDispatcher->dispatchCustomEvent("LifeUpdate", &opHealth);
}

void PlayerNode::update(float delta)
{
    Node::update(delta);
    
    fixedAnimationInterval += AnimationRate*delta;
    for (; fixedAnimationInterval > 1; fixedAnimationInterval -= 1)
    {
        currentAnimation = (currentAnimation + 1) % 30;
        for (auto jet : jetFlames) jet->setSpriteFrame("JetFire" + ulongToString(currentAnimation+1) + ".png");
    }
    
    if (withShooter)
    {
        shooterTimer += delta;
        while (shooterTimer > 1.0f)
        {
            shootProjectile();
            shooterTimer -= 1.0f;
        }
    }
    
    if (damage || invincible)
    {
        damageTimer += delta;
        while (damageTimer > 2*BlinkTime) damageTimer -= 2*BlinkTime;
    }
    else if (damageTimer < 2*BlinkTime) damageTimer += delta;
    
    setOpacity(160 + 95*std::min(fabsf(damageTimer - BlinkTime)/BlinkTime, 1.0f));
    
    if (!touching && motionProcessor)
    {
        auto velocity = motionProcessor->getDirectionVector() * UserDefault::getInstance()->getIntegerForKey("TiltSensitivity");
        auto pos = getPosition() + velocity;
        
        auto size = static_cast<GameScene*>(getParent()->getParent())->getPlayfieldSize();
        pos.clamp(Vec2(44, 44), Vec2(size.width/2, size.height-44));
        setPosition(pos);
        
        auto angle = (velocity/delta + Vec2(1200, 0)).getAngle();
        setRotation(-CC_RADIANS_TO_DEGREES(angle));
        
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        debugOrientationLabel->setString(longToString(motionProcessor->getScreenRotation()));
#endif
    }
}

static const Vector<SpriteFrame*> &getAnimationFrames()
{
    static Vector<SpriteFrame*> frames(24);
    if (frames.empty())
    {
        for (int i = 0; i < 24; i++)
        {
            std::string name = "ShieldAnimation" + ulongToString(i+1) + ".png";
            frames.pushBack(SpriteFrameCache::getInstance()->getSpriteFrameByName(name));
        }
    }
    return frames;
}

void PlayerNode::addShield()
{
    if (onShield) return;
    
    auto glowingBorder = Sprite::createWithSpriteFrameName("ShieldGlowingBorder.png");
    auto blink = EaseSineInOut::create(FadeTo::create(0.4, 150));
    auto blink2 = EaseSineInOut::create(FadeTo::create(0.4, 255));
    
    auto animation = Animation::createWithSpriteFrames(getAnimationFrames(), 1.0/60.0);
    auto animationAction = Animate::create(animation);
    
    auto animationFront = Sprite::create();
    auto animationBack = Sprite::create();
    
    glowingBorder->runAction(RepeatForever::create(Sequence::createWithTwoActions(blink, blink2)));
    animationFront->runAction(RepeatForever::create(animationAction));
    animationBack->runAction(RepeatForever::create(animationAction->reverse()));
    
    for (Sprite *spr : { glowingBorder, animationFront, animationBack })
    {
        spr->setOpacity(0);
        spr->runAction(FadeIn::create(0.5));
        spr->setPosition(getContentSize()/2);
    }
    
    addChild(animationBack, -4, SHIELD_BACK);
    addChild(animationFront, 2, SHIELD_FRONT);
    addChild(glowingBorder, -5, SHIELD_BORDER);
    
    CollisionManager::setPlayer(CollisionManager::PlayerCollisionData(this, Vec2::ZERO, 40, CC_CALLBACK_1(PlayerNode::takeShieldDamage, this),
                                                                      CC_CALLBACK_1(PlayerNode::projectileDamage, this)));
    
    onShield = true;
}

void PlayerNode::changeShieldIcon(PowerupIcon *icon)
{
    shieldIcon = icon;
}

void PlayerNode::removeShield(bool exploding)
{
    if (!onShield) return;
    
    for (int i : { SHIELD_BORDER, SHIELD_BACK, SHIELD_FRONT })
    {
        if (exploding) getChildByTag(i)->runAction(ScaleTo::create(0.5, 2.0));
        getChildByTag(i)->runAction(Sequence::createWithTwoActions(FadeOut::create(0.5), RemoveSelf::create()));
    }
    
    shieldIcon = nullptr;
    updateCollisionData();
    
    onShield = false;
}

inline static float projectedDirDuration(Vec2 dir, Vec2 pos, float radius, Size size)
{
    float duration = INFINITY;
    
    if (dir.x > 0) duration = std::min(duration, (size.width+radius - pos.x)/dir.x);
    else if (dir.x < 0) duration = std::min(duration, -(pos.x+radius)/dir.x);
    
    if (dir.y > 0) duration = std::min(duration, (size.height+radius - pos.y)/dir.y);
    else if (dir.y < 0) duration = std::min(duration, -(pos.y+radius)/dir.y);
    
    return duration;
}

void PlayerNode::shootProjectile()
{
    auto projectile = Sprite::createWithSpriteFrameName("HazardShooterShot.png");
    
    auto transform = getParent()->getParentToNodeAffineTransform();
    auto pos = PointApplyAffineTransform(getChildByTag(SHOOT_DRONE)->convertToWorldSpaceAR(Vec2::ZERO), transform);
    auto dir = 480 * Vec2::forAngle(-CC_DEGREES_TO_RADIANS(getRotation()));
    
    auto duration = projectedDirDuration(dir, pos, 16, static_cast<GameScene*>(getParent()->getParent())->getPlayfieldSize());
    
    projectile->setPosition(pos);
    projectile->runAction(Sequence::createWithTwoActions(MoveBy::create(duration, dir * duration), RemoveSelf::create()));
    
    getParent()->addChild(projectile);
    
    CollisionManager::addProjectile(CollisionManager::ProjectileCollisionData(projectile, Vec2::ZERO, 10));
    SoundManager::play("common/PowerupShotSound.wav");
}

void PlayerNode::addShooter()
{
    if (withShooter) return;
    
    const Size &size = getContentSize();
    
    Node* drone = getChildByTag(SHOOT_DRONE);
    if (drone == nullptr)
    {
        drone = Sprite::createWithSpriteFrameName("HazardShooterShot.png");
        drone->setPosition(size/2);
        addChild(drone, -3, SHOOT_DRONE);
    }
    else drone->stopAllActions();
    
    auto time = (drone->getPositionX() - size.width/2)/48;
    drone->runAction(MoveTo::create(1.0 - time, Vec2(48, 0) + size/2));
    
    withShooter = true;
    shooterTimer = -1.0;
}

void PlayerNode::removeShooter()
{
    if (!withShooter) return;
    
    auto drone = getChildByTag(SHOOT_DRONE);
    drone->runAction(Sequence::create(MoveTo::create(1.0, getContentSize()/2), RemoveSelf::create(), nullptr));
    
    withShooter = false;
    shooterTimer = 0.0;
}

void PlayerNode::turnInvincibility()
{
    invincible = true;
    runAction(Sequence::create(DelayTime::create(8.0), CallFunc::create([this] { invincible = false; }), nullptr));
}