//
//  HazardSelector Spawners.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 16/03/15.
//
//

#include "HazardSelector.h"
#include "CollisionManager.h"
#include "BezierNode.h"
#include "Defaults.h"
#include "CustomActions.h"
#include "GameScene.h"
#include "SoundManager.h"

using namespace cocos2d;

inline static CallFunc *postScore(int value)
{
    return CallFunc::create([value]
                            {
                                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("ScoreUpdate", const_cast<int*>(&value));
                            });
}

inline static CallFunc *moveOffscreen(HazardSelector *obj, Vec2 dir, RefPtr<Action> nextAction)
{
    return CallFuncN::create([=] (Node *node) { obj->moveHazardOffscreen(node, dir, nextAction); });
}

// HAZARD FUNCTIONS
inline static int genSpriteArray(bool *val, int maxSprites)
{
    auto numSprites = random_int_closed(1, maxSprites);
    auto direction = random_int_closed(0, 1) == 0 ? maxSprites-1 : 1;
    
    for (int i = 0; i < numSprites; i++)
    {
        int chosen = random_int_open(0, maxSprites);
        while (val[chosen]) chosen = (chosen + direction) % maxSprites;
        val[chosen] = true;
        direction = random_int_closed(0, 1) == 0 ? maxSprites-1 : 1;
    }
    
    auto currentCount = 0;
    // Check for "one hole" gaps and for whole walls
    for (int i = 0; i < maxSprites-2; i++)
    {
        if (val[i] && !val[i+1] && val[i+2]) val[i+2] = false;
        if (val[i]) currentCount++;
    }
    if (val[maxSprites-2]) currentCount++;
    if (val[maxSprites-1]) currentCount++;
    
    if (currentCount >= maxSprites-1)
    {
        auto index = val[0] ? random_int_open(0, currentCount-1) : random_int_open(1, maxSprites-1);
        val[index] = val[index+1] = false;
        currentCount -= 2;
    }
    
    return currentCount;
}

#define RUN_AND_ADD(obj, act) (obj)->runAction(self->addAction(act))
static bool spriteChoices[12];
void hazardCubeSpawner1(HazardSelector* self)
{
    constexpr float MoveSpeed = 112.5,
                    RotateTime = 2.0,
                    PeriodMin = 1.2,
                    PeriodMax = 3.0,
                    AmplitudeMin = 8,
                    AmplitudeMax = 32;
    
    const Size &size = self->getPlayfieldSize();
    
    int maxSprites = size.height/48;
    auto offset = (size.height - maxSprites*48)/2 + 24;
    
    auto amplitude = random_float_closed(AmplitudeMin, AmplitudeMax);
    auto period = random_float_closed(PeriodMin, PeriodMax);
    
    for (int i = 0; i < maxSprites; i++) spriteChoices[i] = false;
    auto generatedSprites = genSpriteArray(spriteChoices, maxSprites);
    
    for (int i = 0; i < maxSprites; i++)
    {
        if (!spriteChoices[i]) continue;
        auto sprite = Sprite::createWithSpriteFrameName("HazardCube.png");
        sprite->setPosition(size.width + 24, offset + i*48);
        
        auto orgY = offset + i*48;
        auto oscillate = ExecFunc::create(period, [orgY, amplitude] (Node* target, float time)
                                          { target->setPositionY(orgY + amplitude * sin(2 * M_PI * time)); });
        
        RUN_AND_ADD(sprite, RepeatForever::create(oscillate));
        RUN_AND_ADD(sprite, RepeatForever::create(RotateBy::create(RotateTime, 360)));
        
        self->moveHazardOffscreen(sprite, Vec2(-MoveSpeed, 0), Sequence::createWithTwoActions(postScore(30), RemoveSelf::create()), ExitDir::LEFT);
        self->addChild(sprite);
        
        CollisionManager::HazardCollisionData::HazardInfo info = { 30, 30, false, true, { nullptr, nullptr } };
        CollisionManager::addHazard(CollisionManager::HazardCollisionData::createCircle(sprite, Vec2::ZERO, 20, info));
    }
    
    self->spawnTime = 1.0 + 2.0f*float(generatedSprites-1)/(maxSprites-1);
}

void hazardCubeSpawner2(HazardSelector *self)
{
    constexpr float MoveSpeed = 112.5, RotateTime = 2.0f;
    
    const Size &size = self->getPlayfieldSize();
    auto numChains = random_float_closed(0, 1) <= 0.6 ? 2 : 3;
    
    float joinHeight = 42 * (numChains-1);
    float height = 48 * numChains;
    float chainHeight = height/(numChains-1);
    
    float centerY = random_float_closed(48 + height/2, size.height - 48 - height/2);
    
    // Create chains
    BezierNode *nodes[3];
    for (int i = 0; i < numChains-1; i++)
    {
        nodes[i] = BezierNode::create(Color3B(200, 200, 200), 1.5);
        
        nodes[i]->setPosition(size.width + 24, centerY - joinHeight/2 + 42*i + 21);
        nodes[i]->reinputCurve(Vec2(0, -21), Vec2(0, 0), Vec2(0, 21));
        
        auto readjust = ExecFunc::create(1, [=] (Node *node, float time)
        {
            auto bezier = static_cast<BezierNode*>(node);
            auto cvector = 1.5*Vec2(chainHeight/2 - 21, 0)*time*time*time;
            auto variation = time * (chainHeight/2 - 21);
            bezier->reinputCurve(Vec2(0, -21 - variation), cvector, Vec2(0, 21 + variation));
        });
        
        auto node = nodes[i];
        auto addBody = CallFunc::create([=]
        {
            Vec2 points[10];
            
            auto a = Vec2(0, -chainHeight/2);
            auto b = Vec2(1.5*(chainHeight/2 - 21), 0);
            auto c = Vec2(0, chainHeight/2);
            
            for (int j = 0; j < 5; j++)
            {
                auto t = j*0.25f;
                auto p1 = a + t*(b-a), p2 = b + t*(c-b), p = p1 + t*(p2-p1);
                points[j] = p;
                points[9-j] = p + Vec2(2, 0);
            }
            
            CollisionManager::HazardCollisionData::HazardInfo info = { 30, 50, false, true, { nullptr, nullptr } };
            CollisionManager::addHazard(CollisionManager::HazardCollisionData::createPolygon(node, Vec2(0, 0), points, 10, info, true));
        });
        
        auto centerDir = i*(chainHeight-42) + (joinHeight + chainHeight - height)/2 - 21;
        auto spawn = Spawn::create(readjust, MoveBy::create(1, Vec2(0, centerDir)), nullptr);
        
        RUN_AND_ADD(nodes[i], Sequence::create(DelayTime::create(size.width/5/MoveSpeed), spawn, addBody, nullptr));
        
        self->moveHazardOffscreen(nodes[i], Vec2(-MoveSpeed, 0), Sequence::createWithTwoActions(postScore(50), RemoveSelf::create()), ExitDir::LEFT);
        self->addChild(nodes[i]);
    }
    
    for (int i = 0; i < numChains; i++)
    {
        auto sprite = Sprite::createWithSpriteFrameName("HazardCube.png");
        sprite->setPosition(size.width + 24, centerY - joinHeight/2 + i*42);

        RUN_AND_ADD(sprite, RepeatForever::create(RotateBy::create(RotateTime, 360)));
        
        if (!(numChains == 3 && i == 1))
        {
            auto dir = i*(chainHeight-42) + (joinHeight - height)/2;
            auto action = EaseSineInOut::create(MoveBy::create(1, Vec2(0, dir)));
            
            RUN_AND_ADD(sprite, Sequence::create(DelayTime::create(size.width/5/MoveSpeed), action, nullptr));
        }
        
        self->moveHazardOffscreen(sprite, Vec2(-MoveSpeed, 0), Sequence::createWithTwoActions(postScore(30), RemoveSelf::create()), ExitDir::LEFT);
        self->addChild(sprite);
        
        CollisionManager::HazardCollisionData::HazardInfo info = { 30, 30, false, true, { i >= 1 ? nodes[i-1] : nullptr, i < numChains-1 ? nodes[i] : nullptr } };
        CollisionManager::addHazard(CollisionManager::HazardCollisionData::createCircle(sprite, Vec2(0, 0), 20, info));
    }
    
    self->spawnTime = 0.6 + 0.6*numChains;
}

static const Vector<SpriteFrame*> &getAnimationFrames()
{
    static Vector<SpriteFrame*> frames(45);
    if (frames.empty())
    {
        for (int i = 0; i < 45; i++)
        {
            std::string name = "HazardShooter" + ulongToString(i+1) + ".png";
            frames.pushBack(SpriteFrameCache::getInstance()->getSpriteFrameByName(name));
        }
    }
    return frames;
}

static const Vector<SpriteFrame*> &getShootingFrames()
{
    static Vector<SpriteFrame*> frames(15);
    if (frames.empty())
    {
        for (int i = 45; i < 60; i++)
        {
            std::string name = "HazardShooter" + ulongToString(i+1) + ".png";
            frames.pushBack(SpriteFrameCache::getInstance()->getSpriteFrameByName(name));
        }
    }
    return frames;
}

void hazardShooterSpawner(HazardSelector *self)
{
    const float probabilities[] = { 1.0, 0.4, 0.1, 0.05, 0.02, 0.004, 0.001, 0.0001 };
    constexpr int probabilitiesSize = sizeof(probabilities)/sizeof(probabilitiesSize);
    
    const Color3B possibleColors[] = { 0x4CAF50_c3, 0xCDDC39_c3, 0xFFEB3B_c3, 0xFF9800_c3, 0xF44336_c3 };
    constexpr int possibleColorsSize = sizeof(possibleColors)/sizeof(possibleColors[0]);
    
    constexpr float EnterSpeed = 100.0, MoveAmount = 80.0, ExitSpeed = 180.0;

    const Size &size = self->getPlayfieldSize();
    auto fromBottom = random_float_open(0, 1) < 0.5;
    
    auto animate = Animate::create(Animation::createWithSpriteFrames(getAnimationFrames(), 1.0/60));
    auto shootingAnimate = Animate::create(Animation::createWithSpriteFrames(getShootingFrames(), 1.0/60));
    
    auto node = Sprite::createWithSpriteFrameName("HazardShooter1.png");
    node->setAnchorPoint(Vec2(76.0/144, 0.5));
    node->setPosition(random_float_closed(size.width/2, size.width - 40), fromBottom ? -40 : size.height + 40);
    node->setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("ShooterProgram"));
    
    auto background = Sprite::createWithSpriteFrameName("HazardShooterBackground.png");
    background->setPosition(18.5, 16);
    background->setAnchorPoint(node->getAnchorPoint());
    node->addChild(background, -2);
    
    auto playerParent = self->getParent();
    node->schedule([=] (float dt)
    {
        auto player = playerParent->getChildByName("PlayerNode");
        auto pos = player ? player->getPosition() : Vec2(0, size.height/2);
        node->setRotation(180 - CC_RADIANS_TO_DEGREES((pos - node->getPosition()).getAngle()));
    }, "Orient");
    
    auto move1 = MoveBy::create(MoveAmount/EnterSpeed, Vec2(0, fromBottom ? MoveAmount : -MoveAmount));
    auto move2 = moveOffscreen(self, Vec2(random_float_open(0, 1) < 0.1 ? -ExitSpeed : ExitSpeed, 0), RemoveSelf::create());
    
    int numShots = 0;
    {
        float rand = powf(random_float_open(0, 1), self->currentTime/450);
        while (numShots < probabilitiesSize && rand < probabilities[numShots]) numShots++;
    }
    
    Vector<FiniteTimeAction*> actions(6);
    actions.pushBack(move1); actions.pushBack(animate);
    
    auto dualAnimation = Sequence::create(shootingAnimate, shootingAnimate->reverse(), nullptr);
    
    for (int i = 0; i < numShots; i++)
    {
        auto shotsprite = Sprite::createWithSpriteFrameName("HazardShooterShot.png");
        shotsprite->setName("ShotNode" + ulongToString(i));
        shotsprite->setPosition(18.5, 16);
        shotsprite->setLocalZOrder(-1);
        
        node->addChild(shotsprite);
        
        auto spawnProjectile = CallFunc::create([=]
        {
            if (!self->onTitle) SoundManager::play("common/ShooterSound.wav");
            
            auto pos = self->getPlayerPos(Vec2(0, size.height/2));
            
            auto dir = (pos - node->getPosition()).getNormalized();
            auto newPos = node->getPosition() + dir * 58;
            
            shotsprite->setPosition(newPos);
            shotsprite->retain(); // <---------------------+
            shotsprite->removeFromParent();             // |
            node->getParent()->addChild(shotsprite);    // | - safeguard
            shotsprite->release(); // <--------------------+
            CollisionManager::HazardCollisionData::HazardInfo info = { 10, -1, false, false, { nullptr, nullptr } };
            CollisionManager::addHazard(CollisionManager::HazardCollisionData::createCircle(shotsprite, Vec2::ZERO, 3, info));
            
            self->moveHazardOffscreen(shotsprite, dir * 600, Sequence::createWithTwoActions(postScore(50), RemoveSelf::create()));
        });
        
        auto projDelay = DelayTime::create(11.0/64);
        auto projMove = MoveBy::create(5.0/64, Vec2(-46.875, 0));
        auto afterMove = DelayTime::create(0.25);
        
        auto seq = RunOnChild::create(shotsprite->getName(), Sequence::create(projDelay, projMove, spawnProjectile, afterMove, nullptr));
        
        actions.pushBack(Spawn::createWithTwoActions(seq, dualAnimation));
    }
    node->setColor(possibleColors[std::min(numShots, possibleColorsSize)-1]);
    
    actions.pushBack(animate->reverse());
    actions.pushBack(move2);
    
    RUN_AND_ADD(node, Sequence::create(actions));
    
    self->addChild(node);
    
    CollisionManager::HazardCollisionData::HazardInfo info = { 40, numShots*80, false, true, { nullptr, nullptr } };
    CollisionManager::addHazard(CollisionManager::HazardCollisionData::createCircle(node, Vec2::ZERO, 16, info));
    
    self->spawnTime = random_float_closed(0.2, 2);
}

static const Vector<SpriteFrame*> &getJetFireFrames()
{
    static Vector<SpriteFrame*> frames(30);
    if (frames.empty())
    {
        for (int i = 0; i < 30; i++)
        {
            std::string name = "JetFire" + ulongToString(i+1) + ".png";
            frames.pushBack(SpriteFrameCache::getInstance()->getSpriteFrameByName(name));
        }
    }
    return frames;
}

void hazardBulletSpawner(HazardSelector *self)
{
    const Size &size = self->getPlayfieldSize();
    
    auto sprite = Sprite::createWithSpriteFrameName("HazardBullet.png");
    auto pos = self->getPlayerPos(Vec2(0, size.height/2));
    
    float deviation = 120*expf(-self->currentTime/600);
    sprite->setPosition(size.width+36, random_float_closed(std::max(0.0f, pos.y - deviation), std::min(size.height, pos.y + deviation)));
    
    constexpr float Power = 3.0, Inverse = 1/Power, AccelV = 40;
    auto duration = powf((size.width+72)/AccelV, Inverse);
    auto action = EaseCubicActionIn::create(MoveBy::create(duration, Vec2(-size.width-72, 0)));
    
    if (!self->onTitle) SoundManager::play("common/BulletFlight.wav");
    RUN_AND_ADD(sprite, Sequence::create(action, postScore(150), RemoveSelf::create(), nullptr));
    self->addChild(sprite);
    
    auto flame = Sprite::createWithSpriteFrameName("JetFire1.png");
    flame->setPosition(38, 6);
    flame->setRotation(180);
    flame->setScale(0.1875);
    
    auto animate = Animate::create(Animation::createWithSpriteFrames(getJetFireFrames(), 1.0/30));
    flame->runAction(RepeatForever::create(animate));
    sprite->addChild(flame, -1);
    
    CollisionManager::HazardCollisionData::HazardInfo info = { 40, 300, false, true, { nullptr, nullptr } };
    CollisionManager::addHazard(CollisionManager::HazardCollisionData::createTwoOffsetCapsule(sprite, Vec2(-13, 0), Vec2(13, 0), 3, info));
    
    self->spawnTime = 0.2;
}

//void hazardWandererSpawner(HazardSelector *self);

void hazardFireballSpawner(HazardSelector *self)
{
    constexpr float ShipSpeed = 400, PrevFlameSpeed = 200, ExplodeFlameSpeed = 160, FlameInterval = 0.02;
    constexpr int NumFlames = 8;
    
    const Size &size = self->getPlayfieldSize();
    
    auto sprite = Sprite::createWithSpriteFrameName("HazardFireball.png");
    sprite->setPosition(size.width + 36, random_float_closed(36, size.height-36));
    
    Vec2 dir(random_float_closed(36, 240) - sprite->getPositionX(), 0);
    auto duration = -dir.x/ShipSpeed;
    
    int numFlames = 0;
    auto move = MoveBy::create(duration, dir);
    auto spawn = CallFunc::create([=] () mutable
    {
        auto dir = Vec2::forAngle(random_float_closed(-.15, .15)) * PrevFlameSpeed;
        
        auto sp = Sprite::createWithSpriteFrameName("HazardFireballFlame.png");
        sp->setPosition(sprite->getPosition());
        self->moveHazardOffscreen(sp, dir, RemoveSelf::create());
        
        float duration = MAX(0.0, ShipSpeed/PrevFlameSpeed * numFlames++ * FlameInterval - 1.75);
        
        RUN_AND_ADD(sp, Sequence::create(DelayTime::create(duration), TintBy::create(0.6, -8, -22, -129),
                                         ScaleTo::create(0.4, 0), RemoveSelf::create(), nullptr));
        
        self->addChild(sp);
        
        CollisionManager::HazardCollisionData::HazardInfo info = { 20, -1, false, false, { nullptr, nullptr } };
        CollisionManager::addHazard(CollisionManager::HazardCollisionData::createCircle(sp, Vec2::ZERO, 7.5, info));
    });
    
    auto block = CallFunc::create([=]
    {
        if (!self->onTitle) SoundManager::play("common/FireballSplit.wav");
        
        auto vector = Vec2::forAngle(2*M_PI/NumFlames);
        Vec2 dir(ExplodeFlameSpeed, 0);
        
        for (int i = 0; i < NumFlames; i++)
        {
            auto sp = Sprite::createWithSpriteFrameName("HazardFireballFlame.png");
            sp->setPosition(sprite->getPosition());
            
            self->moveHazardOffscreen(sp, dir, RemoveSelf::create());
            
            self->addChild(sp);
            
            CollisionManager::HazardCollisionData::HazardInfo info = { 20, -1, false, false, { nullptr, nullptr } };
            CollisionManager::addHazard(CollisionManager::HazardCollisionData::createCircle(sp, Vec2::ZERO, 7.5, info));
            
            dir = dir.rotate(vector);
        }
    });
    
    RUN_AND_ADD(sprite, Sequence::create(move, block, postScore(100), RemoveSelf::create(), nullptr));
    RUN_AND_ADD(sprite, RepeatForever::create(Sequence::createWithTwoActions(DelayTime::create(FlameInterval), spawn)));
    
    self->addChild(sprite);
    
    CollisionManager::HazardCollisionData::HazardInfo info = { 40, 180, false, true, { nullptr, nullptr } };
    CollisionManager::addHazard(CollisionManager::HazardCollisionData::createCircle(sprite, Vec2::ZERO, 12, info));
    
    self->spawnTime = random_float_closed(0.2, 1.5);
}

static const Vec2 missilePoints[] =
{
    { 18, 0 }, { -14, 7 }, { -15.65, 6.7 }, { -16.85, 5.55 }, { -17.5, 4 },
    { -17.5, -4 }, { -16.85, -5.55 }, { -15.65, -6.7 }, { -14, -7 }
};
static constexpr int missilePointsSize = sizeof(missilePoints)/sizeof(missilePoints[0]);

void hazardMissileSpawner(HazardSelector *self)
{
    const Size &size = self->getPlayfieldSize();
    
    bool fromSides = random_float_open(0, 1) < 0.4;

    int number = 1;
    if (!fromSides) number++;
    
    float prob = random_float_open(0, 1);
    if (prob < 0.4) number++;
    if (prob < 0.1) number++;

    float divisor;
    if (fromSides) divisor = number == 1 ? 0 : (size.width*0.325 - 12)/(number-1);
    else divisor = (size.height - 24)/(number-1);
    
    if (fromSides) number *= 2;
    
    class updateFunctor
    {
        float time;
        Node *node;
        
    public:
        updateFunctor(Node *node) : node(node), time(1.8) {}
        
        void operator()(float dt)
        {
            HazardSelector *hzs = static_cast<HazardSelector*>(node->getParent());
            const Size &size = hzs->getPlayfieldSize();
            float delta = dt * hzs->speed;
            
            if (time > 0)
            {
                time -= delta;
                
                Vec2 pos = hzs->getPlayerPos(Vec2(0, size.height/2));
                
                float angle = -CC_RADIANS_TO_DEGREES((pos - node->getPosition()).getAngle());
                float rotation = node->getRotation();
                
                if (angle - rotation > 180) rotation += 360;
                else if (rotation - angle > 180) rotation -= 360;
                
                rotation += 3.0 * delta * (angle - rotation);
                node->setRotation(rotation);
            }
            
            node->setPosition(node->getPosition() + 250 * delta * Vec2::forAngle(CC_DEGREES_TO_RADIANS(-node->getRotation())));
            
            if (!Rect(-32, -32, size.width+64, size.height+64).containsPoint(node->getPosition()))
            {
                int value = 100;
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("ScoreUpdate", &value);
                node->removeFromParent();
            }
        }
    };
    
    for (int i = 0; i < number; i++)
    {
        auto sprite = Sprite::createWithSpriteFrameName("HazardGuidedMissile.png");
        
        if (fromSides)
        {
            sprite->setPosition(size.width * 0.675 + (i/2) * divisor, i%2 == 0 ? -18 : size.height + 18);
            sprite->setRotation((i%2 + 1) * 180 + 90);
        }
        else
        {
            sprite->setPosition(size.width + 18, 12 + i * divisor);
            sprite->setRotation(180);
        }
        
        sprite->schedule(updateFunctor(sprite), "Update");
        self->addChild(sprite);
        
        CollisionManager::HazardCollisionData::HazardInfo info = { 20, 160, true, true, { nullptr, nullptr } };
        CollisionManager::addHazard(CollisionManager::HazardCollisionData::createPolygon(sprite, Vec2::ZERO, missilePoints, missilePointsSize, info));
    }
    
    self->spawnTime = random_float_closed(1.6, 4);
}

// HAZARD DECLARATION
const HazardSpawner hazardSpawners[] =
{
    { hazardCubeSpawner1,   3.0,  0.0, 0.0 },
    { hazardCubeSpawner2,   3.0,  6.0, 0.0 },
    { hazardShooterSpawner, 2.0, 20.0, 1.0 },
    { hazardBulletSpawner,  3.0, 40.0, 0.2 },
    { hazardFireballSpawner, 1.7, 80.0, 1.0 },
    { hazardMissileSpawner, 0.8, 140.0, 2.0 },
};
const int hazardSpawnerSize = sizeof(hazardSpawners)/sizeof(hazardSpawners[0]);
const std::string hazardStrings[] =
{
    "Avoid cubes and other hazards",
    "",
    "Shooters will shoot on you",
    "Bullets come at blazing speeds",
    "Fireballs explode on your face",
    "Missiles follow you everywhere",
};
