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
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "GPGManager.h"
#endif

ScoreManager::TimeConstraint ScoreManager::currentTimeConstraint = ScoreManager::TimeConstraint::DAILY;
ScoreManager::SocialConstraint ScoreManager::currentSocialConstraint = ScoreManager::SocialConstraint::GLOBAL;
ScoreManager::Source ScoreManager::currentSource = ScoreManager::Source::FACEBOOK;

void ScoreManager::init()
{
    currentTimeConstraint = TimeConstraint::DAILY;
    currentSocialConstraint = SocialConstraint::GLOBAL;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    currentSource = Source::GAME_CENTER;
#else

    currentTimeConstraint = TimeConstraint::ALL;
    currentSocialConstraint = SocialConstraint::FRIENDS;
#endif
}

void ScoreManager::loadPlayerCurrentScore(std::function<void(const ScoreData&)> handler)
{
    switch (currentSource)
    {
        //case Source::EXAMPLE: ExampleScoreManager::loadPlayerCurrentScore(handler); break;
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        case Source::GAME_CENTER: GameCenterManager::loadPlayerCurrentScore(handler); break;
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        case Source::GOOGLE_PLAY_GAMES: GPGManager::loadPlayerCurrentScore(handler); break;
#endif
        case Source::FACEBOOK: FacebookManager::loadPlayerCurrentScore(handler); break;
        default: break;
    }
}

void ScoreManager::loadHighscoresOnRange(long first, long last, std::function<void(long, std::vector<ScoreData>&&, std::string)> handler)
{
    switch (currentSource)
    {
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        case Source::GAME_CENTER: GameCenterManager::loadHighscoresOnRange(currentSocialConstraint, currentTimeConstraint, first, last, handler); break;
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        case Source::GOOGLE_PLAY_GAMES: GPGManager::loadHighscoresOnRange(currentSocialConstraint, currentTimeConstraint, first, last, handler); break;
#endif
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
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    GPGManager::reportScore(global_GameScore);
#endif
    FacebookManager::reportScore(global_GameScore);
}

struct CompareScores
{
    bool operator()(const ScoreManager::ScoreData &s1, const ScoreManager::ScoreData &s2) const { return s1.score < s2.score; }
};

static std::mutex scoreBuildLock;
static std::set<ScoreManager::ScoreData, CompareScores> scoreTracking, tempBuildScore;
static uint64_t sourcesFetched;
static ScoreManager::ScoreData savedScore;

inline void fetchScores(ScoreManager::Source source, long position, std::vector<ScoreManager::ScoreData> &&data, std::string error)
{
	scoreBuildLock.lock();

    sourcesFetched |= 1 << (uint8_t)source;
    std::move(data.begin(), data.end(), std::inserter(tempBuildScore, tempBuildScore.end()));
    
    if (ScoreManager::trackedScoresReady()) scoreTracking = std::move(tempBuildScore);

	scoreBuildLock.unlock();
}

void ScoreManager::updateScoreTrackingArray()
{
    sourcesFetched = 0;
    savedScore = ScoreData();
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    GameCenterManager::loadHighscoresOnRange(SocialConstraint::FRIENDS, TimeConstraint::ALL, 1, INT32_MAX, CC_CALLBACK_3(fetchScores, Source::GAME_CENTER), false);
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	GPGManager::loadHighscoresOnRange(SocialConstraint::FRIENDS, TimeConstraint::ALL, 1, INT32_MAX, CC_CALLBACK_3(fetchScores, Source::GOOGLE_PLAY_GAMES), false);
#endif
    FacebookManager::loadHighscoresOnRange(SocialConstraint::FRIENDS, TimeConstraint::ALL, 1, INT32_MAX, CC_CALLBACK_3(fetchScores, Source::FACEBOOK), false);
}

bool ScoreManager::trackedScoresReady()
{
    return sourcesFetched == (1 << (uint8_t)ScoreManager::Source::NUMBER_OF_SOURCES) - 1;
}

ScoreManager::ScoreData ScoreManager::getNextTrackedScore(int64_t score)
{
    if (score >= savedScore.score)
	{
		scoreBuildLock.lock();

		auto it = scoreTracking.upper_bound(ScoreData(0, "", score));
		if (it == scoreTracking.end()) savedScore = ScoreData(-1, "", INT64_MAX);
		else savedScore = *it;

		scoreBuildLock.unlock();
	}
    
    return savedScore;
}