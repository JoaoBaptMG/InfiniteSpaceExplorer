//
//  ResultNode.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 21/04/15.
//
//

#include "ResultNode.h"
#include "ui/CocosGUI.h"
#include "ScoreNode.h"
#include "GameScene.h"
#include "Defaults.h"
#include "MultiPurposeScene.h"
#include "ScoreManager.h"
#include "AchievementManager.h"
#include "MessageDialog.h"
#include "FacebookManager.h"

using namespace cocos2d;

constexpr auto LATO_REGULAR = "fonts/Lato/Lato-Regular.ttf";
constexpr auto LATO_LIGHT = "fonts/Lato/Lato-Light.ttf";

inline static ui::Button *createButton(const std::string &name)
{
    return ui::Button::create(name + ".png", name + "Hover.png", name + ".png", ui::Widget::TextureResType::PLIST);
}

ResultNode* ResultNode::create(const Size &screenSize)
{
    ResultNode *pRet = new(std::nothrow) ResultNode();
    if (pRet && pRet->init(screenSize))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = NULL;
        return NULL;
    }
}

bool ResultNode::init(const Size &screenSize)
{
    if (!Node::init())
        return false;
    
    auto background = LayerColor::create(Color4B(0, 0, 0, 160));
    background->setScaleY(0);
    background->runAction(Sequence::create(DelayTime::create(1.8), ScaleTo::create(0.8, 1), CallFunc::create(CC_CALLBACK_0(ResultNode::sequence, this)), nullptr));
    
    repushing = false;
    
    addChild(background);
    
    return true;
}

void ResultNode::sequence()
{
    struct { char buttonName[30], string[10]; std::function<void()> method; }
    buttons[] =
    {
        { "ButtonContinue", "RETRY", CC_CALLBACK_0(ResultNode::repushScene, this) },
        { "ButtonFacebook", "FACEBOOK", CC_CALLBACK_0(ResultNode::fbAction, this) },
        { "ButtonExit", "EXIT", CC_CALLBACK_0(ResultNode::popToMain, this) },
    };
    constexpr int buttonCount = sizeof(buttons)/sizeof(buttons[0]);
    
    auto size = getScene()->getContentSize();
    float contentSpacing = 84*(buttonCount-1);
    auto firstPos = size.width/2 - contentSpacing/2;
    
    for (int i = 0; i < buttonCount; i++)
    {
        auto button = createButton(buttons[i].buttonName);
        button->setPosition(Vec2(firstPos + i*84, size.height*0.3));
        addChild(button);
        
        auto label = Label::createWithTTF(buttons[i].string, LATO_REGULAR, 12);
        label->setPosition(button->getPosition() - Vec2(0, 48));
        addChild(label);
        
        auto method = buttons[i].method;
        
        button->setCascadeOpacityEnabled(true);
        label->setCascadeOpacityEnabled(true);
        
        button->setOpacity(0);
        label->setOpacity(0);
        
        auto addListener = CallFunc::create([button,method]()
        {
            button->addTouchEventListener([method] (Ref* button, ui::Widget::TouchEventType type)
                                          {
                                              if (type == ui::Widget::TouchEventType::ENDED)
                                              {
                                                  method();
                                              }
                                          });
        });
        button->runAction(Sequence::createWithTwoActions(FadeIn::create(0.8), addListener));
        label->runAction(FadeIn::create(0.8));
        
        if (i == 1)
        {
            button->setName("FacebookButton");
            label->setName("FacebookLabel");
            
            if (FacebookManager::hasPermission("publish_actions"))
                label->setString("SHARE");
            else label->setString("ALLOW");
        }
    }
    
    auto scoreText = Label::createWithTTF("000000", LATO_LIGHT, 96);
    scoreText->setPosition(Vec2(size.width/2, size.height*.7));
	addChild(scoreText);
    
    auto infoText = Label::createWithTTF("YOUR SCORE ON THIS GAME WAS", LATO_REGULAR, 16);
    infoText->setPosition(scoreText->getPosition() + Vec2(0, 50));
    addChild(infoText);
    
    scoreText->setCascadeOpacityEnabled(true);
    infoText->setCascadeOpacityEnabled(true);
    
    scoreText->setOpacity(0);
    infoText->setOpacity(0);
    
    auto duration = cbrtf(global_GameScore)/20;
    scoreText->runAction(Sequence::create(FadeIn::create(0.8), DelayTime::create(0.8), TextUpdateAction::create(0, global_GameScore, duration), nullptr));
    infoText->runAction(FadeIn::create(0.8));
    
    if (!FacebookManager::hasPermission("publish_actions"))
    {
        auto facebookLabel = Label::createWithTTF("You can tap the button below to allow\nthe game to publish on Facebook", LATO_REGULAR, 16);
        facebookLabel->setAlignment(TextHAlignment::CENTER);
        facebookLabel->setPosition(size/2);
        addChild(facebookLabel);
        
        facebookLabel->setName("FacebookAllowLabel");
        
        facebookLabel->setOpacity(0);
        facebookLabel->runAction(Sequence::createWithTwoActions(DelayTime::create(1.6 + duration), FadeIn::create(0.3)));
    }
}

void ResultNode::repushScene()
{
    if (repushing) return;
    repushing = true;
    
    auto layer = getScene()->getChildByName<GameScene*>("SceneLayer");
    
    RefPtr<ResultNode> val(this);
    auto resetAction = CallFunc::create([=] { layer->reset(); val->removeFromParent(); });

    auto black = LayerColor::create(Color4B(0, 0, 0, 255));
    black->setOpacity(0);
    black->runAction(Sequence::create(FadeIn::create(0.4), resetAction, FadeOut::create(0.4), RemoveSelf::create(), nullptr));

    layer->addChild(black);
}

void ResultNode::popToMain()
{
    scheduleOnce([this] (float delta)
    {
        auto transition = TransitionFade::create(0.8, createSceneWithLayer(MultiPurposeLayer::createTitleScene(BackgroundColor)));
        Director::getInstance()->replaceScene(transition);
    }, 0.01, "ReplaceScene");
}

void ResultNode::fbAction()
{
    if (!FacebookManager::hasPermission("publish_actions"))
    {
        RefPtr<ResultNode> thisPtr = this;
        FacebookManager::requestPublishPermissions([=] (FacebookManager::PermissionState state, std::string)
        {
            if (FacebookManager::hasPermission("publish_actions") || state == FacebookManager::PermissionState::ACCEPTED)
            {
                thisPtr->getChildByName<Label*>("FacebookLabel")->setString("SHARE");
                FacebookManager::reportScore(global_GameScore);
                thisPtr->getChildByName("FacebookAllowLabel")->runAction(Sequence::createWithTwoActions(FadeOut::create(0.3), RemoveSelf::create()));
            }
            else if (state == FacebookManager::PermissionState::ERROR)
            {
                thisPtr->getChildByName<Label*>("FacebookAllowLabel")->setString("There was an error while processing the request.");
            }
        });
    }
    else
        presentMessage("Do you want to share your score on Facebook?", "Share", "Yes", "No", CC_CALLBACK_0(ResultNode::fbShareScore, this), [] {});
}

void ResultNode::fbShareScore()
{
    getChildByName<Label*>("FacebookLabel")->setString("SHARING");
    getChildByName("FacebookButton")->setOpacity(51);
    getChildByName<ui::Button*>("FacebookButton")->setEnabled(false);
    
    std::unordered_map<std::string, std::string> parameters;
    parameters.emplace("link", "https://fb.me/1642009619415245");
    parameters.emplace("name", "Infinite Space Explorer");
    parameters.emplace("description", "I just got " + longToString(global_GameScore) + " points!");
    parameters.emplace("caption", "Play Infinite Space Explorer for free on iOS and Android!");
    
    RefPtr<ResultNode> thisPtr = this;
    FacebookManager::graphRequest("me/feed", parameters, HTTPMethod::POST,
    [=] (Value &&result, std::string error)
    {
        if (result.getType() == Value::Type::MAP)
        {
            thisPtr->getChildByName<Label*>("FacebookLabel")->setString("DONE");
        }
        else
        {
            thisPtr->getChildByName("FacebookButton")->setOpacity(255);
            thisPtr->getChildByName<ui::Button*>("FacebookButton")->setEnabled(true);
            thisPtr->getChildByName<Label*>("FacebookLabel")->setString("ERROR");
        }
    });
}