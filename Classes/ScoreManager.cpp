//
//  ScoreManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 03/05/15.
//
//

#include "ScoreManager.h"
#include "cocos2d.h"
#include "Defaults.h"
#include "ExampleScoreManager.h"
#include "FacebookManager.h"
#include <algorithm>
#include <iterator>

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#include "GameCenterManager.h"
#endif

ScoreManager::TimeConstraint ScoreManager::currentTimeConstraint = TimeConstraint::DAILY;
ScoreManager::SocialConstraint ScoreManager::currentSocialConstraint = SocialConstraint::GLOBAL;
ScoreManager::Source ScoreManager::currentSource = Source::FACEBOOK;

void ScoreManager::init()
{
    currentTimeConstraint = TimeConstraint::DAILY;
    currentSocialConstraint = SocialConstraint::GLOBAL;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    currentSource = Source::GAME_CENTER;
#else
    //currentSource = Source::EXAMPLE;
    currentTimeConstraint = TimeConstraint::ALL;
    currentSocialConstraint = SocialConstraint::FRIENDS;
#endif
    
    //ExampleScoreManager::reportScore(700000);
}

void ScoreManager::loadPlayerCurrentScore(std::function<void(const ScoreData&)> handler)
{
    switch (currentSource)
    {
        //case Source::EXAMPLE: ExampleScoreManager::loadPlayerCurrentScore(handler); break;
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        case Source::GAME_CENTER: GameCenterManager::loadPlayerCurrentScore(handler); break;
#endif
        //case Source::GOOGLE_PLAY_SERVICES: break;
        case Source::FACEBOOK: FacebookManager::loadPlayerCurrentScore(handler); break;
        default: break;
    }
}

void ScoreManager::loadHighscoresOnRange(long first, long last, std::function<void(long, std::vector<ScoreData>&&, std::string)> handler)
{
    switch (currentSource)
    {
        //case Source::EXAMPLE: ExampleScoreManager::loadHighscoresOnRange(currentSocialConstraint, currentTimeConstraint, first, last, handler); break;
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        case Source::GAME_CENTER: GameCenterManager::loadHighscoresOnRange(currentSocialConstraint, currentTimeConstraint, first, last, handler); break;
#endif
        //case Source::GOOGLE_PLAY_SERVICES: break;
        case Source::FACEBOOK: FacebookManager::loadHighscoresOnRange(currentSocialConstraint, currentTimeConstraint, first, last, handler); break;
        default: break;
    }
}

void ScoreManager::reportScore()
{
    ExampleScoreManager::reportScore(global_GameScore);
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    GameCenterManager::reportScore(global_GameScore);
#endif
    //case Source::GOOGLE_PLAY_SERVICES: break;
    FacebookManager::reportScore(global_GameScore);
}

struct CompareScores
{
    bool operator()(const ScoreManager::ScoreData &s1, const ScoreManager::ScoreData &s2) { return s1.score < s2.score; }
};

static std::set<ScoreManager::ScoreData, CompareScores> scoreTracking, tempBuildScore;
static uint64_t sourcesFetched;
static ScoreManager::ScoreData savedScore;

inline void fetchScores(ScoreManager::Source source, long position, std::vector<ScoreManager::ScoreData> &&data, std::string error)
{
    sourcesFetched |= 1 << (uint8_t)source;
    std::move(data.begin(), data.end(), std::inserter(tempBuildScore, tempBuildScore.end()));
    
    if (ScoreManager::trackedScoresReady()) scoreTracking = std::move(tempBuildScore);
}

void ScoreManager::updateScoreTrackingArray()
{
    sourcesFetched = 0;
    savedScore = ScoreData();
    
    //ExampleScoreManager::loadHighscoresOnRange(SocialConstraint::FRIENDS, TimeConstraint::ALL, 1, INT32_MAX, CC_CALLBACK_3(fetchScores, Source::EXAMPLE));
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    GameCenterManager::loadHighscoresOnRange(SocialConstraint::FRIENDS, TimeConstraint::ALL, 1, INT32_MAX, CC_CALLBACK_3(fetchScores, Source::GAME_CENTER), false);
#endif
    FacebookManager::loadHighscoresOnRange(SocialConstraint::FRIENDS, TimeConstraint::ALL, 1, INT32_MAX, CC_CALLBACK_3(fetchScores, Source::FACEBOOK), false);
}

bool ScoreManager::trackedScoresReady()
{
    return sourcesFetched == (1 << (uint8_t)ScoreManager::Source::NUMBER_OF_SOURCES) - 1;
}

ScoreManager::ScoreData ScoreManager::getNextTrackedScore(int64_t score)
{
    if (score < savedScore.score) return savedScore;
    
    auto it = scoreTracking.upper_bound(ScoreData(0, "", score));
    if (it == scoreTracking.end()) return ScoreData(-1, "", INT64_MAX);
    
    return savedScore = *it;
}