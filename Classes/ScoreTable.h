//
//  ScoreTable.h
//  SpaceExplorer
//
//  Created by João Baptista on 17/06/15.
//
//

#ifndef __SpaceExplorer__ScoreTable__
#define __SpaceExplorer__ScoreTable__

#include <vector>
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "ScoreManager.h"
#include "DownloadedPhotoNode.h"

class ScoreWidget : public cocos2d::Node
{
    DownloadedPhotoNode *playerPicture;
    cocos2d::ui::Scale9Sprite *rankBubble;
    cocos2d::Label *rankNumber, *rankSuffix, *nameText, *scoreText;
    
public:
    static ScoreWidget *create(float screenWidth);
    bool init(float screenWidth);
    void updateScoreData(const ScoreManager::ScoreData& data);
};

class ScoreTable : public cocos2d::Node
{
    cocos2d::Size preferredSize;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    cocos2d::EventListenerCustom *gameCenterLoadedListener;
#endif
    
    cocos2d::EventListenerTouchOneByOne *timeConstraintButtonListeners[(int)ScoreManager::TimeConstraint::NUMBER_OF_TIME_CONSTRAINTS];
    cocos2d::EventListenerTouchOneByOne *socialConstraintButtonListeners[(int)ScoreManager::SocialConstraint::NUMBER_OF_SOCIAL_CONSTRAINTS];
    
    cocos2d::Label *timeConstraintButtons[(int)ScoreManager::TimeConstraint::NUMBER_OF_TIME_CONSTRAINTS];
    cocos2d::Label *socialConstraintButtons[(int)ScoreManager::SocialConstraint::NUMBER_OF_SOCIAL_CONSTRAINTS];
    
    cocos2d::ui::Button *sourceButtons[(int)ScoreManager::Source::NUMBER_OF_SOURCES];
    
    cocos2d::LayerColor *timeConstraintDecorator, *socialConstraintDecorator, *sourceDecorator;
    cocos2d::Label *infoLabel, *scoresTopLabel;
    
    cocos2d::ui::ScrollView *canvasView;
    
    ScoreWidget* *fixedWidgetList;
    std::size_t fixedWidgetListSize;
    
    std::vector<ScoreManager::ScoreData> scoreList;
    long lastPosition, optCurrentIndex;
    int currentRequestCode;
    
    float scaling;
    
    long firstLoaded;
    bool hasScoresTop, hasScoresBottom;
    bool scoreRequestedTop, scoreRequestedBottom;
    bool requestSent;
    
    void createConstraintButons(const cocos2d::Vec2 &halfSize);
    void positionDecorators(const cocos2d::Vec2 &halfSize);
    void changeSocialConstraint(ScoreManager::SocialConstraint constraint, bool update = true);
    void changeTimeConstraint(ScoreManager::TimeConstraint constraint, bool update = true);
    void changeSource(ScoreManager::Source source);
    
    cocos2d::EventListenerTouchOneByOne* configureAsButton(cocos2d::Label *label, std::function<void()> handler);

    void redrawScores();
    void scoreCallback(long first, long expectedLast, std::vector<ScoreManager::ScoreData> &&vector, std::string errorString);
    
    void drawScrollView();
    void scrollViewListener(cocos2d::Ref *scrollView, cocos2d::ui::ScrollView::EventType event);
    
    void sendAdditiveRequest(long first, long last, bool before);
    void additiveScoreCallback(long first, long expectedLast, std::vector<ScoreManager::ScoreData> &&vector, std::string errorString, bool before);
public:
    inline void setPreferredSize(cocos2d::Size size) { preferredSize = size; }
    inline cocos2d::Size getPreferredSize() const { return preferredSize; }
    
    static ScoreTable *create(cocos2d::Size size);
    bool init(cocos2d::Size size);
    
    virtual ~ScoreTable();
};

#endif /* defined(__SpaceExplorer__ScoreTable__) */
