//
//  MultiPurposeScene.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 28/02/15.
//
//

#include "MultiPurposeScene.h"
#include "Defaults.h"
#include "GameScene.h"
#include "MessageDialog.h"
#include "ScoreTable.h"
#include "ShipConfig.h"
#include "CustomActions.h"
#include "BlurFilter.h"
#include "HazardSelector.h"
#include "GameCenterManager.h"
#include "FacebookManager.h"
#include "FacebookLoginButton.h"
#include "SoundManager.h"
#include "OpenURL.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "GPGLoginButton.h"
#endif

using namespace cocos2d;

constexpr auto LATO_REGULAR = "fonts/Lato/Lato-Regular.ttf";
constexpr auto LATO_LIGHT = "fonts/Lato/Lato-Light.ttf";
constexpr auto LATO_BOLD = "fonts/Lato/Lato-Bold.ttf";

inline static ui::Button *createButton(const std::string &name)
{
    return ui::Button::create(name + ".png", name + "Hover.png", name + ".png", cocos2d::ui::Widget::TextureResType::PLIST);
}

inline static ui::Button *createHoverButton(const std::string &name)
{
    return ui::Button::create(name + ".png", name + ".png", name + ".png", cocos2d::ui::Widget::TextureResType::PLIST);
}

inline static bool isShipUnlocked(unsigned long shipId)
{
    return UserDefault::getInstance()->getIntegerForKey("AccumulatedScore") >= getShipConfig(shipId).pointsRequired;
}

inline static std::string getLockedText(unsigned long shipId)
{
    if (isShipUnlocked(shipId)) return "AVAILABLE TO PLAY";
    else
    {
        int required = getShipConfig(shipId).pointsRequired - UserDefault::getInstance()->getIntegerForKey("AccumulatedScore");
        return ulongToString(required) + " MORE TO UNLOCK";
    }
}

MultiPurposeLayer* MultiPurposeLayer::create(Color3B color, bool pause)
{
    MultiPurposeLayer *pRet = new(std::nothrow) MultiPurposeLayer();
    if (pRet && pRet->init(color, pause))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        return nullptr;
    }
}

bool MultiPurposeLayer::init(Color3B color, bool pause)
{
    if (!LayerColor::initWithColor((Color4B)color))
        return false;
    
    containerNode = Node::create();
    currentLayout = 0;
    
    addChild(LayerColor::create(Color4B(51, 51, 51, 127)), 6);
    addChild(containerNode, 7);
    
    if (pause) createLayout1Pause();
    else createLayout1Title();
    createLayout2();
    createLayout3();
    
    return true;
}

MultiPurposeLayer::~MultiPurposeLayer()
{
    _director->getTextureCache()->removeTextureForKey("SavedImage");
}

static const Vector<SpriteFrame*> &getJetAnimationFrames()
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

static Vector<Sprite*> getShipSprites(bool animateFlames = false)
{
    static Vector<Sprite*> sprites(getShipConfigSize());
    static Vector<Sprite*> flamesToAnimate {};
    
    if (sprites.empty())
    {
        for (int i = 0; i < getShipConfigSize(); i++)
        {
            const ShipConfig& config = getShipConfig(i);
            
            Sprite *spr = Sprite::createWithSpriteFrameName("PlayerShape" + ulongToString(i) + ".png");
            
            auto spriteFrame = SpriteFrameCache::getInstance()->getSpriteFrameByName("PlayerShape" + ulongToString(i) + "b.png");
            if (spriteFrame)
            {
                auto backSprite = Sprite::createWithSpriteFrame(spriteFrame);
                backSprite->setPosition(spr->getContentSize()/2);
                spr->addChild(backSprite, -2);
            }
            
            for (Vec2 pos : config.jetPositions)
            {
                auto jet = Sprite::createWithSpriteFrameName("JetFire1.png");
                jet->setPosition(Vec2(2.0*pos.x, 2.0*pos.y));
                jet->setScale(2.0*config.jetScale);
                spr->addChild(jet, -1);
                
                flamesToAnimate.pushBack(jet);
            }
            
            sprites.pushBack(spr);
        }
    }
    
    if (animateFlames)
    {
        for (Sprite* jet : flamesToAnimate)
        {
            auto animation = Animate::create(Animation::createWithSpriteFrames(getJetAnimationFrames(), 1.0f/30));
            jet->runAction(RepeatForever::create(animation));
        }
    }
    
    return sprites;
}

void MultiPurposeLayer::createLayout1Title()
{
    auto size = getContentSize();
    size.width -= 48;
    
    // Background
    auto hazard = HazardSelector::create(true);
    hazard->setScale(size.height/StandardPlayfieldHeight);
    BlurNode *blur = BlurNode::create(_director->getWinSize(), hazard);
    addChild(blur, 0);
    
    hazard->artificiallyAdvance(3600);
    
    auto titleLabel = Sprite::createWithSpriteFrameName("GameName.png");
    titleLabel->setPosition(size.width/2, size.height*.84);
    containerNode->addChild(titleLabel);
    
    auto shipLabel = Sprite::createWithSpriteFrameName("PlayerName" + ulongToString(global_ShipSelect) + ".png");
    Vec2 pos(size.width/2, size.height/2 - 104);
    shipLabel->setPosition(pos);
    
    auto pointsRequiredLabel = Label::createWithTTF(getLockedText(global_ShipSelect), LATO_BOLD, 12);
    pos -= Vec2(0, 24);
    pointsRequiredLabel->setPosition(floorf(pos.x), floorf(pos.y));
    
    float radius = size.width * M_SQRT1_2;
    Vec2 center(-16, (size.width + size.height)/2);
    float changeAngle = M_PI_4 + acosf(-64/radius);
    
    for (int i = 0; i < getShipConfigSize(); i++)
    {
        containerNode->addChild(getShipSprites().at(i), -1);
        getShipSprites().at(i)->setVisible(false);
    }
    
    getShipSprites(true).at(global_ShipSelect)->setVisible(true);
    getShipSprites().at(global_ShipSelect)->setPosition(center + Vec2(radius, -radius) * M_SQRT1_2);
    getShipSprites().at(global_ShipSelect)->setRotation(-45);
    
    log("%g", getShipSprites().at(global_ShipSelect)->getPositionX());
    
    auto unlockedButton = createButton("ButtonPlayUnlocked");
    unlockedButton->setPosition(Vec2(56, 0) + size/2);
    unlockedButton->addTouchEventListener([=] (Ref* button, ui::Widget::TouchEventType type)
    {
        if (type == ui::Widget::TouchEventType::BEGAN)
            SoundManager::play("common/ClickButton.wav");
        else if (type == ui::Widget::TouchEventType::ENDED)
        {
            auto transition = TransitionFade::create(0.8, createSceneWithLayer(GameScene::create()));
            Director::getInstance()->replaceScene(transition);
        }
    });
    
    auto lockedButton = createButton("ButtonPlayLocked");
    lockedButton->setPosition(unlockedButton->getPosition());
    
#ifdef COCOS2D_DEBUG
    lockedButton->addTouchEventListener([=] (Ref* button, ui::Widget::TouchEventType type)
      {
          if (type == ui::Widget::TouchEventType::BEGAN)
              SoundManager::play("common/ClickButton.wav");
          else if (type == ui::Widget::TouchEventType::ENDED)
          {
              auto transition = TransitionFade::create(0.8, createSceneWithLayer(GameScene::create()));
              Director::getInstance()->replaceScene(transition);
          }
      });
#endif
    
    
    if (isShipUnlocked(global_ShipSelect)) lockedButton->setScale(0);
    else unlockedButton->setScale(0);
    
    std::string name = "SelectionArrow.png";
    ui::Button *arrowButtons[2];
    arrowButtons[0] = ui::Button::create(name, name, name, ui::Widget::TextureResType::PLIST);
    arrowButtons[1] = ui::Button::create(name, name, name, ui::Widget::TextureResType::PLIST);
    
	auto arrowButton0 = arrowButtons[0], arrowButton1 = arrowButtons[1];

    int i = 0;
    for (int dir : { -1, +1 })
    {
        auto arrowButton = arrowButtons[i++];
        
        arrowButton->setScaleX(dir);
        arrowButton->setPosition(Vec2(dir*128, 0) + size/2);
        
        arrowButton->setHighlighted(true);
        arrowButton->getVirtualRenderer()->setColor(Color3B(200, 200, 200));
        arrowButton->setHighlighted(false);
        arrowButton->setContentSize(cocos2d::Size(32, 32));
        
        arrowButton->addTouchEventListener([=] (Ref *target, ui::Widget::TouchEventType type)
        {
            if (type == ui::Widget::TouchEventType::BEGAN)
                SoundManager::play("common/ClickButton.wav");
            else if (type == ui::Widget::TouchEventType::ENDED)
            {
				for (auto arrow : { arrowButton0, arrowButton1 }) arrow->setEnabled(false);
                
                auto currentShip = global_ShipSelect;
                auto nextShip = (global_ShipSelect + dir + getShipConfigSize()) % getShipConfigSize();
                getShipSprites().at(nextShip)->setVisible(true);
                
                auto rotate = ExecFunc::create(1.4, [=] (Node*, float time)
                {
                    float angleLeaving = -M_PI_4 + time*changeAngle;
                    getShipSprites().at(currentShip)->setPosition(center + radius * Vec2::forAngle(angleLeaving));
                    getShipSprites().at(currentShip)->setRotation(-90 - CC_RADIANS_TO_DEGREES(angleLeaving));
                    
                    float angleComing = angleLeaving - changeAngle;
                    getShipSprites().at(nextShip)->setPosition(center + radius * Vec2::forAngle(angleComing));
                    getShipSprites().at(nextShip)->setRotation(-90 - CC_RADIANS_TO_DEGREES(angleComing));
                });
                
                auto action = CallFunc::create([=]
                {
                    getShipSprites().at(global_ShipSelect)->setVisible(false);
                    global_ShipSelect = nextShip;
                    shipLabel->setSpriteFrame("PlayerName" + ulongToString(nextShip) + ".png");
                    pointsRequiredLabel->setString(getLockedText(nextShip));
                    
                    for (auto arrow : { arrowButton0, arrowButton1 }) arrow->setEnabled(true);
                });
                
                shipLabel->runAction(EaseCubicActionInOut::create(rotate));
                shipLabel->runAction(Sequence::create(FadeOut::create(0.7), action, FadeIn::create(0.7), nullptr));
                pointsRequiredLabel->runAction(Sequence::create(FadeOut::create(0.7), FadeIn::create(0.7), nullptr));
                
                (isShipUnlocked(currentShip) ? unlockedButton : lockedButton)->runAction(ScaleTo::create(0.2, 0.0));
                (isShipUnlocked(nextShip) ? unlockedButton : lockedButton)->runAction(Sequence::createWithTwoActions(DelayTime::create(1.2), ScaleTo::create(0.2, 1.0)));
            }
        });
        
        containerNode->addChild(arrowButton);
    }

    containerNode->addChild(lockedButton, -2);
    containerNode->addChild(unlockedButton, -2);
    containerNode->addChild(shipLabel);
    containerNode->addChild(pointsRequiredLabel);
}

void MultiPurposeLayer::createLayout1Pause()
{
    struct { char buttonName[30], string[10]; std::function<void()> method; }
    buttons[] =
    {
        { "ButtonContinue", "CONTINUE", CC_CALLBACK_0(MultiPurposeLayer::popToGame, this) },
        { "ButtonScores", "SCORES", [this]() { changeLayout(2); } },
        { "ButtonConfig", "SETTINGS", [this]() { changeLayout(1); } },
        { "ButtonExit", "EXIT", CC_CALLBACK_0(MultiPurposeLayer::exitConfirm, this) },
    };
    constexpr int buttonCount = sizeof(buttons)/sizeof(buttons[0]);
    
    auto size = getContentSize();
    float contentSpacing = 84*(buttonCount-1);
    auto firstPos = (size.width - 48)/2 - contentSpacing/2;
    
    for (int i = 0; i < buttonCount; i++)
    {
        auto button = createButton(buttons[i].buttonName);
        button->setPosition(Vec2(firstPos + i*84, size.height*0.25));
        containerNode->addChild(button);
        
        auto label = Label::createWithTTF(buttons[i].string, LATO_REGULAR, 12);
        label->setPosition(button->getPosition() - Vec2(0, 48));
        containerNode->addChild(label);
        
        auto method = buttons[i].method;
        button->addTouchEventListener([method] (Ref* button, ui::Widget::TouchEventType type)
                                      {
                                          if (type == ui::Widget::TouchEventType::BEGAN)
                                              SoundManager::play("common/ClickButton.wav");
                                          else if (type == ui::Widget::TouchEventType::ENDED) method();
                                      });
    }
    
    auto scoreText = Label::createWithTTF(ulongToString(global_GameScore, 6), LATO_LIGHT, 80);
    scoreText->setPosition(Vec2((size.width - 48)/2, size.height*.75));
    containerNode->addChild(scoreText, 2);
    
    auto infoText = Label::createWithTTF("CURRENT SCORE", LATO_REGULAR, 12);
    infoText->setPosition(scoreText->getPosition() + Vec2(0, 40));
    containerNode->addChild(infoText, 3);
}

void MultiPurposeLayer::popToGame()
{
    auto scene = Director::getInstance()->previousScene();
    Director::getInstance()->popScene(TransitionCrossFade::create(0.4, scene));
}

void MultiPurposeLayer::exitConfirm()
{
    presentMessage("Do you really want to quit? Your score will not be recorded", "Already going?", "Yes", "No", [this]
                   {
                       scheduleOnce([this] (float delta)
                                    {
                                        retain();
                                        auto scene = createSceneWithLayer(MultiPurposeLayer::createTitleScene(BackgroundColor));
                                        Director::getInstance()->popScene(TransitionFade::create(0.8, scene));
                                        release();
                                    }, 0.001, "ReplaceScene");
                   }, []{});
}


constexpr struct { const char name[20], text[20], defaultKey[20]; int begin, end; } sliders[] =
{
    { "ButtonMusicOn.png", "MUSIC VOLUME", "MusicVolume", 0, 100 },
    { "ButtonSoundOn.png", "SOUND VOLUME", "SoundVolume", 0, 100 },
    { "ButtonTiltSens.png", "TILT SENSITIVITY", "TiltSensitivity", 20, 96 },
};
constexpr int SliderCount = (sizeof(sliders)/sizeof(sliders[0]));
constexpr float SliderBottomSpacing = 80, SliderTopSpacing = 24;

void MultiPurposeLayer::createLayout2()
{
    auto size = getContentSize();
    
    auto colorRect = LayerColor::create(Color4B::WHITE, size.width, size.height);
    colorRect->setPosition(size.width-48, 0);
    containerNode->addChild(colorRect);
    
    for (int i = 0; i < 2; i++)
    {
        arrowSprites[i] = createButton("PauseScreenArrow");
        arrowSprites[i]->setPosition(Vec2((i+1)*(size.width-48) + 24, size.height/2));
        containerNode->addChild(arrowSprites[i]);
        
        arrowSprites[i]->addTouchEventListener([this, i] (Ref* button, ui::Widget::TouchEventType type)
        {
            if (type == ui::Widget::TouchEventType::BEGAN)
                SoundManager::play("common/ClickButton.wav");
            else if (type == ui::Widget::TouchEventType::ENDED)
                changeLayout(currentLayout == i ? i+1 : i);
        });
    }
    
    auto spacing = (size.height - SliderBottomSpacing - SliderTopSpacing)/SliderCount;
    
    for (auto i = 0; i < SliderCount; i++)
    {
        auto position = SliderBottomSpacing + spacing*(i+0.5);
        
        auto sliderBack = ui::Scale9Sprite::createWithSpriteFrameName("PauseSliderFill.png");
        sliderBack->setPosition(size.width+112, position);
        sliderBack->setScaleX(64/sliderBack->getContentSize().width);
        containerNode->addChild(sliderBack, 100);
        
        auto slider = ui::Slider::create("PauseSliderBackground.png", "PauseSliderKnot.png", ui::Widget::TextureResType::PLIST);
        slider->loadProgressBarTexture("PauseSliderFill.png", ui::Widget::TextureResType::PLIST);
        slider->setPosition(Vec2(1.5*size.width-4, position));
        slider->setScale9Enabled(true);
        slider->Widget::setContentSize(cocos2d::Size(size.width-264, 8));
        containerNode->addChild(slider, 200);
        
        auto sprite = Sprite::createWithSpriteFrameName(sliders[i].name);
        sprite->setPosition(size.width+80, position);
        containerNode->addChild(sprite, 300);
        
        auto value = UserDefault::getInstance()->getIntegerForKey(sliders[i].defaultKey);
        auto label = Label::createWithTTF(std::string(sliders[i].text) + ": " + ulongToString(value), LATO_REGULAR, 8);
        label->setHorizontalAlignment(TextHAlignment::LEFT);
        label->setTextColor(Color4B(102, 102, 102, 255));
        containerNode->addChild(label, 400);
        
        label->setPosition(Vec2(roundf(size.width + label->getContentSize().width/2 + 128), position-24));
        
        auto labelPtr = RefPtr<Label>(label);
        slider->setPercent(100 * float(value-sliders[i].begin)/(sliders[i].end-sliders[i].begin));
        slider->addEventListener([i, labelPtr] (Ref *obj, ui::Slider::EventType type)
        {
            auto slider = static_cast<ui::Slider*>(obj);
            
            auto newValue = sliders[i].begin + int(roundf(slider->getPercent() * (sliders[i].end-sliders[i].begin) / 100.0f));
            UserDefault::getInstance()->setIntegerForKey(sliders[i].defaultKey, newValue);
            labelPtr->setString(std::string(sliders[i].text) + ": " + ulongToString(newValue));
            if (i == 0) SoundManager::updateBackgroundVolume();
        });
    }
    
    auto fbButton = FacebookLoginButton::create();
    fbButton->setPosition(size.width + 48 + fbButton->getContentSize().width/2, SliderBottomSpacing/2);
    containerNode->addChild(fbButton);
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	auto gpgButton = GPGLoginButton::create();
	gpgButton->setPosition(fbButton->getPosition() + Vec2(fbButton->getContentSize().width / 2 + 8 + gpgButton->getContentSize().width / 2, 0));
	containerNode->addChild(gpgButton);
#endif

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    auto gameCenterButton = createHoverButton("IconGameCenter");
    gameCenterButton->setPosition(gpgButton->getPosition() + Vec2(gpgButton->getContentSize().width/2 + 8 + gameCenterButton->getContentSize().width/2, 0));
    containerNode->addChild(gameCenterButton);
    
    gameCenterButton->addTouchEventListener([this] (Ref* button, ui::Widget::TouchEventType type)
    {
        if (type == ui::Widget::TouchEventType::BEGAN)
            SoundManager::play("common/ClickButton.wav");
        else if (type == ui::Widget::TouchEventType::ENDED)
            GameCenterManager::presentWidget();
    });
#endif
    
    auto aboutButton = createHoverButton("ButtonAboutTheCreators");
    aboutButton->setHighlighted(true);
    aboutButton->getVirtualRenderer()->setColor(Color3B(200, 200, 200));
    aboutButton->setHighlighted(false);
    
    aboutButton->setPosition(Vec2(2*(size.width-48) - 4 - aboutButton->getContentSize().width/2, SliderBottomSpacing/2));
    containerNode->addChild(aboutButton);
    
    aboutButton->addTouchEventListener([this] (Ref* button, ui::Widget::TouchEventType type)
    {
        if (type == ui::Widget::TouchEventType::BEGAN)
            SoundManager::play("common/ClickButton.wav");
        else if (type == ui::Widget::TouchEventType::ENDED)
            openURL("http://infinitespaceexplorer.com/");
    });
}

void MultiPurposeLayer::createLayout3()
{
    auto size = getContentSize();
    
    auto colorRect = LayerColor::create(Color4B(240, 240, 240, 255), size.width-48, size.height);
    colorRect->setPosition(2*size.width-48, 0);
    containerNode->addChild(colorRect);
    
    auto table = ScoreTable::create(cocos2d::Size(size.width - 120, size.height - 40));
    table->setPosition(2.5*size.width - 72, size.height/2);
    containerNode->addChild(table);
}

void MultiPurposeLayer::changeLayout(int nextLayout)
{
    if (nextLayout == currentLayout) return;
    
    auto amount = (currentLayout-nextLayout)*(getContentSize().width-48);
    containerNode->runAction(EaseSineOut::create(MoveBy::create(0.6, Vec2(amount, 0))));
    
    if (nextLayout < currentLayout)
        for (int i = nextLayout; i < currentLayout; i++)
            arrowSprites[i]->runAction(RotateBy::create(0.6, +180));
    else
        for (int i = currentLayout; i < nextLayout; i++)
            arrowSprites[i]->runAction(RotateBy::create(0.6, -180));
    
    currentLayout = nextLayout;
}

void MultiPurposeLayer::onEnterTransitionDidFinish()
{
    // Check for some automatic values
    auto rateOnStore = UserDefault::getInstance()->getIntegerForKey("CountdownToRate", 9);
    
    if (rateOnStore > 0) UserDefault::getInstance()->setIntegerForKey("CountdownToRate", rateOnStore - 1);
    else if (rateOnStore == 0)
    {
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        std::string callToAction = "Rate our game on App Store if you like it! It will help us improve your experience.";
        std::string url = "itms-apps://itunes.apple.com/app/id1024737542";
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        std::string callToAction = "Rate our game on Play Store if you like it! It will help us improve your experience.";
        std::string url = "";
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
		std::string callToAction = "Rate our game on Windows Store if you like it! It will help us improve your experience.";
		std::string url = "";
#endif
        
        UserDefault::getInstance()->setIntegerForKey("CountdownToRate", -1);
        presentMessage(callToAction, "Leave us a review!", "Sure!", "Not now...", [=] { openURL(url); }, [] {});
    }
    
    auto likeOnFacebook = UserDefault::getInstance()->getIntegerForKey("CountdownToLike", 14);
    
    if (likeOnFacebook > 0) UserDefault::getInstance()->setIntegerForKey("CountdownToLike", likeOnFacebook - 1);
    else if (likeOnFacebook == 0)
    {
        UserDefault::getInstance()->setIntegerForKey("CountdownToLike", -1);
        presentMessage("We have a Facebook page! From it, you can also get information on your game!", "Follow us on Facebook",
                       "Sure!", "Not now...", [=] { openURL("http://facebook.com/infinite.space.explorer/"); }, [] {});
    }
}