//
//  ScoreTable.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 17/06/15.
//
//

#include "ScoreTable.h"
#include "Defaults.h"

using namespace cocos2d;

constexpr auto LATO_REGULAR = "fonts/Lato/Lato-Regular.ttf";
constexpr auto LATO_LIGHT = "fonts/Lato/Lato-Light.ttf";

constexpr float ScoreTableSpacing = 28, HitAreaExpansion = 8, ScoreWidgetHeight = 56, ScoreWidgetSpacing = 8;
constexpr long ScoreChunkSize = 25;

constexpr auto PullText = "Pull to load more!";
constexpr auto ReleaseText = "Release to load more!";
constexpr auto NoScoreText = "No scores!";
constexpr auto NoMoreScoreText = "No more scores!";
constexpr auto LoadingText = "Loading...";

ScoreWidget* ScoreWidget::create(float screenWidth)
{
    ScoreWidget *pRet = new(std::nothrow) ScoreWidget();
    if (pRet && pRet->init(screenWidth))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool ScoreWidget::init(float screenWidth)
{
    if (!Node::init()) return false;
    
    playerPicture = DownloadedPhotoNode::create();
    playerPicture->setPosition(-screenWidth/2 + 24, 0);
    
    rankBubble = ui::Scale9Sprite::createWithSpriteFrameName("PauseRankBadge.png");
    rankBubble->setCapInsets(Rect(12, 0, 24, 24));
    
    rankNumber = Label::createWithTTF("", LATO_LIGHT, 12);
    rankSuffix = Label::createWithTTF("", LATO_LIGHT, 6);
    
    rankNumber->setTextColor(Color4B(102, 102, 102, 255));
    rankSuffix->setTextColor(Color4B(102, 102, 102, 255));
    
    nameText = Label::createWithTTF("", LATO_REGULAR, 18);
    scoreText = Label::createWithTTF("", LATO_LIGHT, 24);
    
    nameText->setTextColor(Color4B(51, 51, 51, 255));
    scoreText->setTextColor(Color4B(153, 153, 153, 255));
    
    addChild(playerPicture, 60);
    addChild(rankBubble, 70);
    addChild(rankNumber, 80);
    addChild(rankSuffix, 90);
    addChild(nameText, 100);
    addChild(scoreText, 110);
    
    return true;
}

inline std::string indexSuffix(long index)
{
    if (index % 10 == 1 && index % 100 != 11) return "st";
    if (index % 10 == 2 && index % 100 != 12) return "nd";
    if (index % 10 == 3 && index % 100 != 13) return "rd";
    return "th";
}

void ScoreWidget::updateScoreData(const ScoreManager::ScoreData &data)
{
    setVisible(true);
    
    playerPicture->setTextureKey(data.textureKey);
    
    rankNumber->setString(ulongToString(data.index));
    rankSuffix->setString(indexSuffix(data.index));
    
    Size bubbleSize = rankBubble->getOriginalSize();
    float minWidth = bubbleSize.width/2;
    bubbleSize.width = MAX(rankNumber->getContentSize().width + rankSuffix->getContentSize().width + 8, minWidth);
    rankBubble->setPreferredSize(bubbleSize);
    
    rankBubble->setPosition(playerPicture->getPosition() + Vec2(rankBubble->getPreferredSize().width + 18, bubbleSize.height)/2);
    rankNumber->setPosition(rankBubble->getPosition() - Vec2(rankSuffix->getContentSize().width/2, 0));
    rankSuffix->setPosition(rankBubble->getPosition() + Vec2(rankNumber->getContentSize().width/2, 3));
    
    float pos = MAX(rankBubble->getPositionX() + rankBubble->getPreferredSize().width/2 - playerPicture->getPositionX() + 4, 48);
    
    nameText->setString(data.name);
    scoreText->setString(ulongToString(data.score, 6));
    
    nameText->setPosition(playerPicture->getPosition() + nameText->getContentSize()/2 + Vec2(pos, 2));
    scoreText->setPosition(playerPicture->getPosition() + Vec2(scoreText->getContentSize().width/2 + pos, -scoreText->getContentSize().height/2 + 6));
    
    if (data.isPlayer)
    {
        nameText->setTextColor(Color4B(57, 147, 176, 255));
        scoreText->setTextColor(Color4B(136, 190, 208, 255));
    }
    else
    {
        nameText->setTextColor(Color4B(51, 51, 51, 255));
        scoreText->setTextColor(Color4B(153, 153, 153, 255));
    }
}

inline static ui::Button *createButton(const std::string &name)
{
    return ui::Button::create(name + ".png", name + ".png", name + ".png", cocos2d::ui::Widget::TextureResType::PLIST);
}

ScoreTable *ScoreTable::create(Size size)
{
    ScoreTable *pRet = new(std::nothrow) ScoreTable();
    if (pRet && pRet->init(size))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool ScoreTable::init(Size size)
{
    if (!Node::init()) return false;
    
    setPreferredSize(size);
    auto halfSize = Vec2(size)/2;
    
    scaling = MIN(size.width/420, 1);
    
    int i = 0;
    for (auto val : { "DAILY", "WEEKLY", "ALL", })
        timeConstraintButtons[i++] = Label::createWithTTF(val, LATO_REGULAR, 12 * scaling);
    
    i = 0;
    for (auto val : { "FRIENDS", "GLOBAL" })
        socialConstraintButtons[i++] = Label::createWithTTF(val, LATO_REGULAR, 12 * scaling);
    
    i = 0;
    for (auto val : {
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        "IconGameCenter",
		"IconGPG",
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
		"IconGPG",
#endif
        "IconFacebook" })
        if (*val != 0) sourceButtons[i++] = createButton(val);
    
    timeConstraintDecorator = LayerColor::create(Color4B(51, 51, 51, 255), 1, 1);
    socialConstraintDecorator = LayerColor::create(Color4B(51, 51, 51, 255), 1, 1);
    sourceDecorator = LayerColor::create(Color4B(51, 51, 51, 255), 1, 1);
    
    infoLabel = Label::createWithTTF("", LATO_REGULAR, 18, Size(size.width, size.height - scaling * ScoreTableSpacing), TextHAlignment::CENTER, TextVAlignment::CENTER);
    infoLabel->setTextColor(Color4B(153, 153, 153, 255));
    infoLabel->setPosition(size/2);
    infoLabel->retain(); // To prevent being deallocated on its removal
    
    scoresTopLabel = Label::createWithTTF("", LATO_REGULAR, 18, Size(size.width, size.height - scaling * ScoreTableSpacing), TextHAlignment::CENTER, TextVAlignment::CENTER);
    scoresTopLabel->setTextColor(Color4B(153, 153, 153, 255));
    scoresTopLabel->setPosition(size/2);
    scoresTopLabel->retain(); // To prevent being deallocated on its removal
    
    canvasView = ui::ScrollView::create();
    canvasView->setClippingType(ui::Layout::ClippingType::SCISSOR);
    canvasView->setDirection(ui::ScrollView::Direction::VERTICAL);
    canvasView->setBounceEnabled(true);
    canvasView->setScrollBarEnabled(false);
    canvasView->setContentSize(Size(size.width, size.height - scaling * ScoreTableSpacing));
    canvasView->setInnerContainerSize(canvasView->getContentSize());
    canvasView->getInnerContainer()->setCascadeOpacityEnabled(true);
    canvasView->getInnerContainer()->addChild(infoLabel);
    canvasView->setPosition(-halfSize);
    
    fixedWidgetListSize = ceilf((size.height - scaling * ScoreTableSpacing)/ScoreWidgetHeight) + 1;
    fixedWidgetList = new ScoreWidget*[fixedWidgetListSize];
    for (int i = 0; i < fixedWidgetListSize; i++)
    {
        fixedWidgetList[i] = ScoreWidget::create(size.width);
        fixedWidgetList[i]->setPosition(size.width/2, 0);
        fixedWidgetList[i]->setCascadeOpacityEnabled(true);
        fixedWidgetList[i]->retain();
    }
    
    lastPosition = optCurrentIndex = 0;
    currentRequestCode = 0;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    gameCenterLoadedListener = _eventDispatcher->addCustomEventListener("SocialManagersRefreshed", [=] (EventCustom*) { redrawScores(); });
#endif
    
    auto separator = LayerColor::create(Color4B(204, 204, 204, 255), size.width, 1);
    separator->setPosition(-halfSize.x, halfSize.y - scaling * ScoreTableSpacing);
    addChild(separator);
    
    addChild(canvasView);
    
    createConstraintButons(halfSize);
    redrawScores();
    
    return true;
}

ScoreTable::~ScoreTable()
{
    for (int i = 0; i < fixedWidgetListSize; i++)
        fixedWidgetList[i]->release();
    delete[] fixedWidgetList;
 
    infoLabel->release();
    scoresTopLabel->release();
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    _eventDispatcher->removeEventListener(gameCenterLoadedListener);
#endif
}

EventListenerTouchOneByOne* ScoreTable::configureAsButton(Label *label, std::function<void()> handler)
{
    label->setOpacity(51);
    label->setTextColor(Color4B(51, 51, 51, 255));
    
    auto eventListener = EventListenerTouchOneByOne::create();
    eventListener->onTouchBegan = [=] (Touch *touch, Event *event)
    {
        auto size = label->getContentSize();
        if (Rect(-HitAreaExpansion, -HitAreaExpansion, size.width + 2*HitAreaExpansion, size.height + 2*HitAreaExpansion).containsPoint(label->convertTouchToNodeSpace(touch)))
        {
            label->setOpacity(25);
            return true;
        }
        else return false;
    };
    eventListener->onTouchCancelled = [=] (Touch *touch, Event *event) { label->setOpacity(51); };
    eventListener->onTouchEnded = [=] (Touch *touch, Event *event)
    {
        auto size = label->getContentSize();
        if (Rect(-HitAreaExpansion, -HitAreaExpansion, size.width + 2*HitAreaExpansion, size.height + 2*HitAreaExpansion).containsPoint(label->convertTouchToNodeSpace(touch)))
        {
            label->setOpacity(51);
            handler();
        }
    };
    eventListener->onTouchMoved = [=] (Touch *touch, Event *event)
    {
        auto size = label->getContentSize();
        if (Rect(-HitAreaExpansion, -HitAreaExpansion, size.width + 2*HitAreaExpansion, size.height + 2*HitAreaExpansion).containsPoint(label->convertTouchToNodeSpace(touch)))
            label->setOpacity(25);
        else label->setOpacity(51);
    };
    eventListener->setSwallowTouches(true);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(eventListener, label);
    
    return eventListener;
}

void ScoreTable::createConstraintButons(const Vec2 &halfSize)
{
    float accumulated = 2;
    
    for (int i = 0; i < (int)ScoreManager::SocialConstraint::NUMBER_OF_SOCIAL_CONSTRAINTS; i++)
    {
        addChild(socialConstraintButtons[i]);
        
        const Size &size = socialConstraintButtons[i]->getContentSize();
        socialConstraintButtons[i]->setPosition(Vec2(-halfSize.x + accumulated + 8 + size.width/2, halfSize.y + size.height/2 - 20 * scaling));
        accumulated += size.width + 16;
        
        socialConstraintButtonListeners[i] = configureAsButton(socialConstraintButtons[i], [i,this] { changeSocialConstraint(ScoreManager::SocialConstraint(i)); });
    }
    
    float totalSize = accumulated;
    
    accumulated = 2;
    
    for (int i = (int)ScoreManager::TimeConstraint::NUMBER_OF_TIME_CONSTRAINTS; i--;)
    {
        addChild(timeConstraintButtons[i]);
        
        const Size &size = timeConstraintButtons[i]->getContentSize();
        timeConstraintButtons[i]->setPosition(Vec2(halfSize.x - accumulated - 8 - size.width/2, halfSize.y + size.height/2 - 20 * scaling));
        accumulated += size.width + 16;
        
        timeConstraintButtonListeners[i] = configureAsButton(timeConstraintButtons[i], [i,this] { changeTimeConstraint(ScoreManager::TimeConstraint(i)); });
    }
    
    totalSize += accumulated;
    
    float width = 32 * scaling * (int)ScoreManager::Source::NUMBER_OF_SOURCES;
    accumulated = -width/2 + 16 * scaling;
    
    for (int i = 0; i < (int)ScoreManager::Source::NUMBER_OF_SOURCES; i++)
    {
        addChild(sourceButtons[i]);
        
        sourceButtons[i]->setScale(scaling);
        sourceButtons[i]->setOpacity(51);
        sourceButtons[i]->setPosition(Vec2(accumulated, halfSize.y - scaling * (ScoreTableSpacing - 16)));
        accumulated += 32 * scaling;
        
        sourceButtons[i]->addTouchEventListener([i,this] (Ref* button, ui::Widget::TouchEventType type)
        {
            if (type == ui::Widget::TouchEventType::ENDED)
            {
                changeSource(ScoreManager::Source(i));
            }
        });
    }
    
    positionDecorators(halfSize);
    
    changeSocialConstraint(ScoreManager::currentSocialConstraint, false);
    changeTimeConstraint(ScoreManager::currentTimeConstraint, false);
    changeSource(ScoreManager::currentSource);
    
    if (ScoreManager::currentSource == ScoreManager::Source::FACEBOOK)
    {
        for (auto button : { socialConstraintButtons[(int)ScoreManager::SocialConstraint::GLOBAL],
            timeConstraintButtons[(int)ScoreManager::TimeConstraint::DAILY], timeConstraintButtons[(int)ScoreManager::TimeConstraint::WEEKLY] })
            button->setOpacity(0);
    }
}

void ScoreTable::positionDecorators(const Vec2 &halfSize)
{
    auto socialLabel = socialConstraintButtons[(int)ScoreManager::currentSocialConstraint];
    auto timeLabel = timeConstraintButtons[(int)ScoreManager::currentTimeConstraint];
    auto sourceButton = sourceButtons[(int)ScoreManager::currentSource];
    
    socialConstraintDecorator->setScaleX(socialLabel->getContentSize().width + 16);
    timeConstraintDecorator->setScaleX(timeLabel->getContentSize().width + 16);
    sourceDecorator->setScaleX(32 * scaling);
    
    socialConstraintDecorator->setPosition(socialLabel->getPositionX(), halfSize.y - scaling * ScoreTableSpacing);
    timeConstraintDecorator->setPosition(timeLabel->getPositionX(), halfSize.y - scaling * ScoreTableSpacing);
    sourceDecorator->setPosition(sourceButton->getPositionX(), halfSize.y - scaling * ScoreTableSpacing);
    
    addChild(socialConstraintDecorator);
    addChild(timeConstraintDecorator);
    addChild(sourceDecorator);
}

void ScoreTable::changeSocialConstraint(ScoreManager::SocialConstraint constraint, bool update)
{
    socialConstraintButtonListeners[(int)ScoreManager::currentSocialConstraint]->setEnabled(true);
    socialConstraintButtonListeners[(int)constraint]->setEnabled(false);
    
    auto button1 = socialConstraintButtons[(int)ScoreManager::currentSocialConstraint];
    auto button2 = socialConstraintButtons[(int)constraint];
    
    button1->setOpacity(51);
    button2->setOpacity(255);
    
    if (ScoreManager::currentSocialConstraint != constraint)
    {
        socialConstraintDecorator->runAction(EaseSineOut::create(ScaleTo::create(0.5, button2->getContentSize().width + 16, 1.0)));
        socialConstraintDecorator->runAction(EaseSineOut::create(MoveBy::create(0.5, button2->getPosition() - button1->getPosition())));
        
        if (update)
        {
            auto action = Sequence::create(FadeOut::create(0.1), DelayTime::create(0.3), CallFunc::create(CC_CALLBACK_0(ScoreTable::redrawScores, this)), FadeIn::create(0.1), nullptr);
            canvasView->getInnerContainer()->runAction(action);
        }
    }
    
    ScoreManager::currentSocialConstraint = constraint;
}

void ScoreTable::changeTimeConstraint(ScoreManager::TimeConstraint constraint, bool update)
{
    timeConstraintButtonListeners[(int)ScoreManager::currentTimeConstraint]->setEnabled(true);
    timeConstraintButtonListeners[(int)constraint]->setEnabled(false);
    
    auto button1 = timeConstraintButtons[(int)ScoreManager::currentTimeConstraint];
    auto button2 = timeConstraintButtons[(int)constraint];
    
    button1->setOpacity(51);
    button2->setOpacity(255);
    
    if (ScoreManager::currentTimeConstraint != constraint)
    {
        timeConstraintDecorator->runAction(EaseSineOut::create(ScaleTo::create(0.5, button2->getContentSize().width + 16, 1.0)));
        timeConstraintDecorator->runAction(EaseSineOut::create(MoveBy::create(0.5, button2->getPosition() - button1->getPosition())));
        
        if (update)
        {
            auto action = Sequence::create(FadeOut::create(0.1), DelayTime::create(0.3), CallFunc::create(CC_CALLBACK_0(ScoreTable::redrawScores, this)), FadeIn::create(0.1), nullptr);
            canvasView->getInnerContainer()->runAction(action);
        }
    }
    
    ScoreManager::currentTimeConstraint = constraint;
}

void ScoreTable::changeSource(ScoreManager::Source source)
{
    auto button1 = sourceButtons[(int)ScoreManager::currentSource];
    auto button2 = sourceButtons[(int)source];
    
    button1->setEnabled(true);
    button2->setEnabled(false);
    
    button1->setOpacity(51);
    button2->setOpacity(255);
    
    if (ScoreManager::currentSource != ScoreManager::Source::FACEBOOK && source == ScoreManager::Source::FACEBOOK)
    {
        changeSocialConstraint(ScoreManager::SocialConstraint::FRIENDS, false);
        changeTimeConstraint(ScoreManager::TimeConstraint::ALL, false);
        
        for (int c : { (int)ScoreManager::SocialConstraint::GLOBAL })
        {
            socialConstraintButtonListeners[c]->setEnabled(false);
            socialConstraintButtons[c]->runAction(FadeOut::create(0.25));
        }
        for (int c : { (int)ScoreManager::TimeConstraint::DAILY, (int)ScoreManager::TimeConstraint::WEEKLY })
        {
            timeConstraintButtonListeners[c]->setEnabled(false);
            timeConstraintButtons[c]->runAction(FadeOut::create(0.25));
        }
    }
    else if (ScoreManager::currentSource == ScoreManager::Source::FACEBOOK && source != ScoreManager::Source::FACEBOOK)
    {
        for (int c : { (int)ScoreManager::SocialConstraint::GLOBAL })
        {
            socialConstraintButtonListeners[c]->setEnabled(true);
            socialConstraintButtons[c]->runAction(FadeTo::create(0.25, 51));
        }
        for (int c : { (int)ScoreManager::TimeConstraint::DAILY, (int)ScoreManager::TimeConstraint::WEEKLY })
        {
            timeConstraintButtonListeners[c]->setEnabled(true);
            timeConstraintButtons[c]->runAction(FadeTo::create(0.25, 51));
        }
    }
    
    if (ScoreManager::currentSource != source)
    {
        sourceDecorator->runAction(EaseSineOut::create(MoveBy::create(0.5, button2->getPosition() - button1->getPosition())));
        
        auto action = Sequence::create(FadeOut::create(0.1), DelayTime::create(0.3), CallFunc::create(CC_CALLBACK_0(ScoreTable::redrawScores, this)), FadeIn::create(0.1), nullptr);
        canvasView->getInnerContainer()->runAction(action);
    }
    
    ScoreManager::currentSource = source;
}

void ScoreTable::redrawScores()
{
    canvasView->getInnerContainer()->removeAllChildren();
    canvasView->getInnerContainer()->addChild(infoLabel);
    
    infoLabel->setString(LoadingText);
    infoLabel->setPosition(preferredSize/2);
    
    for (int i = 0; i < fixedWidgetListSize; i++) fixedWidgetList[i]->setOpacity(255);
    
    firstLoaded = -1;
    lastPosition = -1;
    optCurrentIndex = 0;
    canvasView->addEventListener(nullptr);
    canvasView->jumpToTop();
    canvasView->setInnerContainerSize(Size(preferredSize.width, preferredSize.height - scaling * ScoreTableSpacing));
    //canvasView->initRenderer();
    
    scoreList.clear();
    
    // This is necessary
    int requestCode = currentRequestCode + 1;
    currentRequestCode = requestCode;
    
    RefPtr<ScoreTable> tblPtr = this;
    ScoreManager::loadHighscoresOnRange(1, ScoreChunkSize, [=] (long first, std::vector<ScoreManager::ScoreData> &&vector, std::string errorString)
    {
        _scheduler->performFunctionInCocosThread([=] () mutable
        {
            if (tblPtr->currentRequestCode == requestCode)
                tblPtr->scoreCallback(first, ScoreChunkSize, std::move(vector), errorString);
            else log("Response for revious request arrived! Current request: %d, previous request: %d", tblPtr->currentRequestCode, requestCode);
            // C++ and is sillies: you need to use "mutable" in order to move a vector to its callback
            // (it makes sense, because move actually modifies the vector)
        });
    });
}

void ScoreTable::scoreCallback(long first, long expectedLast, std::vector<ScoreManager::ScoreData> &&vector, std::string errorString)
{
    //if (thisPtr->getParent() == nullptr) return;
    
    if (!vector.empty())
    {
        canvasView->addEventListener(CC_CALLBACK_2(ScoreTable::scrollViewListener, this));
        canvasView->setInnerContainerSize(Size(preferredSize.width, MAX(vector.size() * ScoreWidgetHeight + ScoreWidgetSpacing, canvasView->getContentSize().height)));
        
        scoreList = std::move(vector);
        
        for (int i = 0; i < fixedWidgetListSize; i++)
            canvasView->getInnerContainer()->addChild(fixedWidgetList[i]);
        
        drawScrollView();
        
        infoLabel->setPositionY(-28);
        if ((hasScoresBottom = scoreList.size() >= expectedLast - first + 1))
            infoLabel->setString(PullText);
        else
            infoLabel->setString(NoMoreScoreText);
        
        canvasView->getInnerContainer()->addChild(scoresTopLabel);
        scoresTopLabel->setPositionY(canvasView->getInnerContainerSize().height + 28);
        
        if ((hasScoresTop = first > 1))
            scoresTopLabel->setString(PullText);
        else
            scoresTopLabel->setString(NoMoreScoreText);
        
        firstLoaded = first;
        scoreRequestedBottom = scoreRequestedTop = false;
    }
    else if (!errorString.empty())
        infoLabel->setString("Error retrieving highscore table: " + errorString);
    else infoLabel->setString(NoScoreText);
}

inline long mod(long a, long b) { return (a + b) % b; }

void ScoreTable::drawScrollView()
{
    if (scoreList.size()*ScoreWidgetHeight <= canvasView->getContentSize().height)
    {
        for (long i = 0; i < scoreList.size(); i++)
        {
            fixedWidgetList[i]->setPositionY(canvasView->getContentSize().height - ScoreWidgetSpacing - ScoreWidgetHeight * (i + 0.5));
            fixedWidgetList[i]->updateScoreData(scoreList[i]);
        }
        
        for (long i = scoreList.size(); i < fixedWidgetListSize; i++)
            fixedWidgetList[i]->setVisible(false);
    }
    else
    {
        for (int i = 0; i < fixedWidgetListSize; i++)
        {
            fixedWidgetList[i]->setPositionY(ScoreWidgetSpacing + ScoreWidgetHeight * (scoreList.size() - i - 0.5));
            if (i < scoreList.size())
                fixedWidgetList[i]->updateScoreData(scoreList[i]);
            else
                fixedWidgetList[i]->setVisible(false);
        }
    }
    
    lastPosition = 0;
}

void ScoreTable::scrollViewListener(Ref *ref, ui::ScrollView::EventType event)
{
    auto scrollView = static_cast<ui::ScrollView*>(ref);
    
    if (event == ui::ScrollView::EventType::SCROLLING)
    {
        if (!scoreList.empty())
        {
            if (scoreList.size()*ScoreWidgetHeight > scrollView->getContentSize().height)
            {
                long currentPosition = (scrollView->getInnerContainerSize().height - canvasView->getContentSize().height + scrollView->getInnerContainer()->getPositionY())/ScoreWidgetHeight;
                currentPosition = MAX(MIN(currentPosition, (long)scoreList.size() - (long)fixedWidgetListSize), 0);

                while (currentPosition != lastPosition)
                {
                    auto optIndexToChange = mod(currentPosition > lastPosition ? optCurrentIndex++ : --optCurrentIndex, fixedWidgetListSize);
                    auto index = currentPosition > lastPosition ? (lastPosition++ + fixedWidgetListSize) : --lastPosition;
                    
                    long position = scoreList.size() - 1 - index;
                    fixedWidgetList[optIndexToChange]->setPositionY(ScoreWidgetSpacing + ScoreWidgetHeight * (position + 0.5));
                    if (index < scoreList.size())
                        fixedWidgetList[optIndexToChange]->updateScoreData(scoreList[index]);
                    else
                        fixedWidgetList[optIndexToChange]->setVisible(false);
                    
                    optCurrentIndex = mod(optCurrentIndex, fixedWidgetListSize);
                }
                
                lastPosition = currentPosition;
            }
        }
        
        if (requestSent)
        {
            scoresTopLabel->setString(LoadingText);
            infoLabel->setString(LoadingText);
        }
        else
        {
            if (hasScoresTop)
            {
                auto cond = scrollView->getInnerContainer()->getPositionY() <= scrollView->getContentSize().height * 2.0 / 3.0 - scrollView->getInnerContainerSize().height;
                
                if (!scoreRequestedTop && cond)
                {
                    scoresTopLabel->setString(ReleaseText);
                    scoreRequestedTop = true;
                }
                else if (scoreRequestedTop && !cond)
                {
                    scoresTopLabel->setString(PullText);
                    scoreRequestedTop = false;
                }
            }
        
            if (hasScoresBottom)
            {
                auto cond = scrollView->getInnerContainer()->getPositionY() >= scrollView->getContentSize().height / 3.0;
                
                if (!scoreRequestedBottom && cond)
                {
                    infoLabel->setString(ReleaseText);
                    scoreRequestedBottom = true;
                }
                else if (scoreRequestedBottom && !cond)
                {
                    infoLabel->setString(PullText);
                    scoreRequestedBottom = false;
                }
            }
        }
    }
    else if (event == ui::ScrollView::EventType::BOUNCE_BOTTOM)
    {
        if (scoreRequestedBottom)
            sendAdditiveRequest(firstLoaded + scoreList.size(), firstLoaded + scoreList.size() + ScoreChunkSize - 1, false);
        scoreRequestedBottom = false;
    }
    else if (event == ui::ScrollView::EventType::BOUNCE_TOP)
    {
        if (scoreRequestedTop)
            sendAdditiveRequest(firstLoaded - ScoreChunkSize, firstLoaded - 1, true);
        scoreRequestedTop = false;
    }
}

void ScoreTable::sendAdditiveRequest(long first, long last, bool before)
{
    // This is necessary
    int requestCode = currentRequestCode + 1;
    currentRequestCode = requestCode;
    
    RefPtr<ScoreTable> tblPtr = this;
    ScoreManager::loadHighscoresOnRange(first, last, [=] (long firstS, std::vector<ScoreManager::ScoreData> &&vector, std::string errorString)
    {
        _scheduler->performFunctionInCocosThread([=] () mutable
        {
            if (tblPtr->currentRequestCode == requestCode)
                tblPtr->additiveScoreCallback(firstS, last, std::move(vector), errorString, before);
            else log("Response for revious request arrived! Current request: %d, previous request: %d", tblPtr->currentRequestCode, requestCode);
        });
    });
}

void ScoreTable::additiveScoreCallback(long first, long expectedLast, std::vector<ScoreManager::ScoreData> &&vector, std::string errorString, bool before)
{
    //if (thisPtr->getParent() == nullptr) return;
    
    auto oldFirst = firstLoaded;
    if (before)
    {
        firstLoaded = first;
        vector.insert(vector.end(), scoreList.begin(), scoreList.end());
        scoreList = std::move(vector);
    }
    else scoreList.insert(scoreList.end(), vector.begin(), vector.end());
    
    canvasView->setInnerContainerSize(Size(preferredSize.width, MAX(scoreList.size() * ScoreWidgetHeight + ScoreWidgetSpacing, canvasView->getContentSize().height)));
    auto position = canvasView->getInnerContainer()->getPositionY();
    canvasView->getInnerContainer()->setPositionY(position + ScoreWidgetHeight * (oldFirst - firstLoaded));
    
    scrollViewListener(canvasView, ui::ScrollView::EventType::SCROLLING);

    if ((hasScoresBottom = scoreList.size() >= expectedLast - firstLoaded + 1))
        infoLabel->setString(PullText);
    else
        infoLabel->setString(NoMoreScoreText);

    scoresTopLabel->setPositionY(canvasView->getInnerContainerSize().height + 28);
    
    if ((hasScoresTop = firstLoaded > 1))
        scoresTopLabel->setString(PullText);
    else
        scoresTopLabel->setString(NoMoreScoreText);
    
    scoreRequestedBottom = scoreRequestedTop = false;
}