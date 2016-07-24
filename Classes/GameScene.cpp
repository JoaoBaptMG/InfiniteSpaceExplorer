//
//  GameScene.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 07/03/15.
//
//

#include "GameScene.h"
#include "PlayerNode.h"
#include "ScoreNode.h"
#include "LifeMarker.h"
#include "TutorialNode.h"
#include "MultiPurposeScene.h"
#include "Defaults.h"
#include "HazardSelector.h"
#include "CollisionManager.h"
#include "ResultNode.h"
#include "PowerupSpawner.h"
#include "BackgroundNode.h"
#include "ScoreManager.h"
#include "BlurFilter.h"
#include "FacebookManager.h"
#include "SoundManager.h"

using namespace cocos2d;

constexpr float FadeTime = 120.0;
const Color3B colors[] = { 0x433A5C_c3, 0x3A485C_c3, 0x43697C_c3, 0x378CA8_c3 };
constexpr int colorsSize = sizeof(colors)/sizeof(colors[0]);

bool GameScene::init()
{
    if (!LayerColor::initWithColor(Color4B(colors[0])))
        return false;
    
    colorID = 0;
    
    addChild(backgroundLayer = Node::create());
    addChild(gameLayer = Node::create());
    addChild(uiLayer = Node::create());
    
    auto size = getContentSize();
    playfieldSize = Size(size.width/size.height * StandardPlayfieldHeight, StandardPlayfieldHeight);
    gameLayer->setScale(size.height/StandardPlayfieldHeight);
    
    uiLayer->addChild(ScoreNode::create(size));
    uiLayer->addChild(LifeMarkers::create(size));
    uiLayer->addChild(TutorialNode::create(size));
    
    backgroundLayer->addChild(BackgroundNode::create());
    
    CollisionManager::clearCollisionData();
    
    scheduleUpdate();
    schedule([] (float delta) { CollisionManager::update(); }, "CollisionUpdate");
    
    gameLayer->addChild(PlayerNode::create());
    gameTime = 0;
    alreadyChecked = alreadyChecked2 = false;
    
    lifeUpdateListener = _eventDispatcher->addCustomEventListener("LifeUpdate", CC_CALLBACK_1(GameScene::lifeUpdate, this));
    
    backgroundListener = _eventDispatcher->addCustomEventListener("DidEnterBackground", CC_CALLBACK_1(GameScene::toBackground, this));
    foregroundListener = _eventDispatcher->addCustomEventListener("WillEnterForeground", CC_CALLBACK_1(GameScene::toForeground, this));
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    gameCenterListener = _eventDispatcher->addCustomEventListener("FreezeForGameCenter", [=] (EventCustom *event) { gotoPauseScreen(); });
#endif
    
    firstRenderTexture = RenderTexture::create(size.width, size.height, Texture2D::PixelFormat::RGBA8888, GL_DEPTH24_STENCIL8);
    targetRenderTexture = RenderTexture::create(size.width/4, size.height/4, Texture2D::PixelFormat::RGBA8888);
    helperRenderTexture = RenderTexture::create(size.width/4, size.height/4, Texture2D::PixelFormat::RGBA8888);
    outputRenderTexture = RenderTexture::create(size.width/4, size.height/4, Texture2D::PixelFormat::RGBA8888);
    
    firstRenderTexture->retain();
    targetRenderTexture->retain();
    helperRenderTexture->retain();
    outputRenderTexture->retain();
    
    firstRenderTexture->getSprite()->setScale(0.25);
    firstRenderTexture->getSprite()->setAnchorPoint(Vec2::ZERO);
    targetRenderTexture->getSprite()->setAnchorPoint(Vec2::ZERO);
    helperRenderTexture->getSprite()->setAnchorPoint(Vec2::ZERO);
    outputRenderTexture->getSprite()->setAnchorPoint(Vec2::ZERO);
    outputRenderTexture->getSprite()->setScale(4);
    outputRenderTexture->getSprite()->getTexture()->setAntiAliasTexParameters();
    
    return true;
}

GameScene::~GameScene()
{
    unschedule("CollisionUpdate");
    _eventDispatcher->removeEventListener(lifeUpdateListener);
    _eventDispatcher->removeEventListener(backgroundListener);
    _eventDispatcher->removeEventListener(foregroundListener);
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    _eventDispatcher->removeEventListener(gameCenterListener);
#endif
    
    firstRenderTexture->release();
    targetRenderTexture->release();
    helperRenderTexture->release();
    outputRenderTexture->release();
}

void GameScene::onEnter()
{
    LayerColor::onEnter();
    
    if (!alreadyChecked2)
    {
        if (UserDefault::getInstance()->getBoolForKey("TutorialFirstPhase"))
            createPauseButton();
        alreadyChecked2 = true;
    }
    
    recursivePause(this);
}

void GameScene::onExitTransitionDidStart()
{
    LayerColor::onExitTransitionDidStart();
    Device::setKeepScreenOn(false);
    recursivePause(this);
}

void GameScene::onEnterTransitionDidFinish()
{
    LayerColor::onEnterTransitionDidFinish();
    Device::setKeepScreenOn(true);
    recursiveResume(this);
    
    if (!alreadyChecked)
    {
        checkTutorialPhase();
        alreadyChecked = true;
    }
}

void GameScene::toBackground(EventCustom *event)
{
    if (Director::getInstance()->getRunningScene() == getScene())
        recursivePause(this);
}

void GameScene::toForeground(EventCustom *event)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WP8)
    auto callback = [this] (float delta)
    {
        unschedule("ComebackEvent");
#endif
        if (Director::getInstance()->getRunningScene() == getScene())
        {
            recursiveResume(this);
            
            if (gameLayer->getChildByName("PlayerNode") != nullptr)
                gotoPauseScreen();
        }
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WP8)
    };
    
    resume();
    schedule(callback, 0, 0, 0.2, "ComebackEvent");
#endif
}

void GameScene::lifeUpdate(EventCustom *event)
{
    auto number = *static_cast<int*>(event->getUserData());
    
    if (number >= 0)
    {
        auto allWhite = LayerColor::create(Color4B::WHITE);
        uiLayer->addChild(allWhite);
        
        allWhite->runAction(Sequence::createWithTwoActions(FadeOut::create(0.8), RemoveSelf::create()));
        
        if (number == 0)
        {
            // Proceed to finalize the game
            gameLayer->removeChildByName("PlayerNode");
            uiLayer->addChild(ResultNode::create(getContentSize()));
            uiLayer->getChildByName<ui::Button*>("PauseButton")->setEnabled(false);
            
            // Post the score
            ScoreManager::reportScore();
            
            // Update the accumulated score
            int accumulatedScore = UserDefault::getInstance()->getIntegerForKey("AccumulatedScore");
            accumulatedScore += global_GameScore;
            UserDefault::getInstance()->setIntegerForKey("AccumulatedScore", accumulatedScore);
        }
    }
}

void GameScene::checkTutorialPhase()
{
    if (!UserDefault::getInstance()->getBoolForKey("TutorialFirstPhase"))
    {
        std::string str = "$Tap to start the ship";
        _eventDispatcher->dispatchCustomEvent("TutorialMessage", &str);
        
        str = "Tilt to move the ship";
        _eventDispatcher->dispatchCustomEvent("TutorialMessage", &str);
        
        str = "#{TutorialDone}$Tap and hold to stabilize";
        _eventDispatcher->dispatchCustomEvent("TutorialMessage", &str);
        
        tutorialDoneListener = _eventDispatcher->addCustomEventListener("TutorialDone", [this] (EventCustom *event)
        {
            gameLayer->addChild(HazardSelector::create());
            gameLayer->addChild(PowerupSpawner::create());
            
            auto pauseButton = createPauseButton();
            pauseButton->setCascadeOpacityEnabled(true);
            pauseButton->setOpacity(0);
            pauseButton->runAction(FadeIn::create(0.3));
            
            _eventDispatcher->removeEventListener(tutorialDoneListener);
            UserDefault::getInstance()->setBoolForKey("TutorialFirstPhase", true);
        });
    }
    else
    {
        gameLayer->addChild(HazardSelector::create());
        gameLayer->addChild(PowerupSpawner::create());
    }
}

ui::Button *GameScene::createPauseButton()
{
    auto pauseButton = ui::Button::create("ButtonPause.png", "ButtonPause.png", "ButtonPause.png", ui::Widget::TextureResType::PLIST);
    pauseButton->setName("PauseButton");
    pauseButton->setHighlighted(true);
    pauseButton->getVirtualRenderer()->setColor(Color3B(200, 200, 200));
    pauseButton->setHighlighted(false);
    
    auto size = getScene()->getContentSize(), bsize = pauseButton->getContentSize();
    pauseButton->setPosition(Vec2(size.width - 4 - bsize.width/2, 4 + bsize.height/2));
    pauseButton->addTouchEventListener(CC_CALLBACK_2(GameScene::pauseButtonPressed, this));
    uiLayer->addChild(pauseButton);
    
    return pauseButton;
}

void GameScene::gotoPauseScreen()
{
    auto size = getScene()->getContentSize();
    auto colorb = getColor();
    auto color = Color4F(colorb);
    
    for (Node *node : getChildren()) recursivePause(node);

    firstRenderTexture->beginWithClear(color.r, color.g, color.b, color.a);
    visit(_director->getRenderer(), _director->getMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW), true);
    firstRenderTexture->end();
    
    targetRenderTexture->begin();
    firstRenderTexture->getSprite()->visit();
    targetRenderTexture->end();
    
    applyBlurFilter(targetRenderTexture, helperRenderTexture, outputRenderTexture, 4);

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WP8
    RefPtr<GameScene> thisPtr = this;
    scheduleOnce([=] (float delta)
    {
        auto layer = MultiPurposeLayer::createPauseScene(colorb);
        
        auto tex = Director::getInstance()->getTextureCache()->addImage(thisPtr->outputRenderTexture->newImage(), "SavedImage");
        auto sprite = Sprite::createWithTexture(tex);
        sprite->setPosition(size/2);
        sprite->setScale(4);
        layer->addChild(sprite);

        auto scene = createSceneWithLayer(layer);
        Director::getInstance()->pushScene(TransitionCrossFade::create(0.4, scene));
    }, 0.05, "PauseSchedule");
#else
    auto layer = MultiPurposeLayer::createPauseScene(colorb);
    layer->addChild(outputRenderTexture);
    
    auto scene = createSceneWithLayer(layer);
    Director::getInstance()->pushScene(TransitionCrossFade::create(0.4, scene));
#endif
}

void GameScene::pauseButtonPressed(Ref *object, ui::Widget::TouchEventType type)
{
    if (type == ui::Widget::TouchEventType::BEGAN)
        SoundManager::play("common/ClickButton.wav");
    else if (type == ui::Widget::TouchEventType::ENDED)
        gotoPauseScreen();
}

void GameScene::reset()
{
    gameTime = 0;
    colorID = 0;
    
    gameLayer->removeAllChildren();
    
    gameLayer->addChild(PlayerNode::create());
    gameLayer->addChild(HazardSelector::create());
    gameLayer->addChild(PowerupSpawner::create());
    
    uiLayer->removeAllChildren();
    
    auto size = getContentSize();
    uiLayer->addChild(ScoreNode::create(size));
    uiLayer->addChild(LifeMarkers::create(size));
    uiLayer->addChild(TutorialNode::create(size));
    createPauseButton();
    
    backgroundLayer->removeAllChildren();
    backgroundLayer->addChild(BackgroundNode::create());
    
    CollisionManager::clearCollisionData();
}

void GameScene::update(float delta)
{
    LayerColor::update(delta);
    
    gameTime += delta;
    while (gameTime >= FadeTime)
    {
        gameTime -= FadeTime;
        colorID = (colorID + 1) % colorsSize;
    }
    
    auto color1 = colors[colorID], color2 = colors[(colorID+1) % colorsSize];
    
    float index = gameTime/FadeTime;
    setColor(Color3B(color1.r + index * (color2.r-color1.r), color1.g + index * (color2.g-color1.g), color1.b + index * (color2.b - color1.b)));
}

void GameScene::visit(Renderer *renderer, const Mat4& parentTransform, uint32_t parentFlags)
{
    LayerColor::visit(renderer, parentTransform, parentFlags);
}