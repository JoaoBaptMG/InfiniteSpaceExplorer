//
//  AchievementManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 17/08/16.
//
//

#include "AchievementManager.h"
#include "cocos2d.h"
#include "Defaults.h"
#include "GPGManager.h"
#include "GameCenterManager.h"

#include <unordered_map>
#include <vector>

using namespace cocos2d;

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
const std::unordered_map<std::string, std::string> achievementMapToGPG
{
	{ "ScoreGot0", "CgkIucWamdkeEAIQCQ" },
	{ "ScoreGot1", "CgkIucWamdkeEAIQCg" },
	{ "ScoreGot2", "CgkIucWamdkeEAIQCw" },
	{ "ScoreGot3", "CgkIucWamdkeEAIQDA" },
	{ "HazardHit0", "CgkIucWamdkeEAIQDQ" },
	{ "HazardHit1", "CgkIucWamdkeEAIQDg" },
	{ "HazardHit2", "CgkIucWamdkeEAIQDw" },
	{ "HazardHit3", "CgkIucWamdkeEAIQEA" },
	{ "PowerupCollected0", "CgkIucWamdkeEAIQEQ" },
	{ "PowerupCollected1", "CgkIucWamdkeEAIQEg" },
	{ "PowerupCollected2", "CgkIucWamdkeEAIQEw" },
	{ "PowerupCollected3", "CgkIucWamdkeEAIQFA" },
	{ "PowerupCollected4", "" },
	{ "GameTime0", "CgkIucWamdkeEAIQFQ" },
	{ "GameTime1", "CgkIucWamdkeEAIQFg" },
	{ "GameTime2", "CgkIucWamdkeEAIQFw" },
	{ "Unlock0", "CgkIucWamdkeEAIQGA" },
	{ "Unlock1", "CgkIucWamdkeEAIQGQ" },
};
#endif

struct StatData
{
	enum class Type { FULL, INCR } type;
	std::vector<int> milestones;
};

std::unordered_map <std::string, StatData> stats
{
	{ "ScoreGot", { StatData::Type::FULL, { 20000, 50000, 100000, 500000 } } },
	{ "HazardHit", { StatData::Type::INCR, { 100, 250, 750, 2000 } } },
	{ "PowerupCollected", { StatData::Type::INCR, { 300, 1000, 2500, 5000, 10000 } } },
	{ "GameTime", { StatData::Type::FULL, { 7, 15, 30 } } },
	{ "Unlock", { StatData::Type::INCR, { 800000, 3000000 } } }
};

void syncStat(std::string stat)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	std::mutex syncMutex;
	std::condition_variable syncCondition;

	int maximumMilestone = stats.find(stat)->second.milestones.size();
	int maxVal = UserDefault::getInstance()->getIntegerForKey(stat.c_str());

    bool fetched = false;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
	GameCenterManager::getAchievementProgress(stat + ulongToString(maximumMilestone-1), [&](double percent)
	{
		std::lock_guard<std::mutex> syncGuard(syncMutex);

		int val = percent * stats.find(stat)->second.milestones.back();
		if (maxVal < val)
			maxVal = val;

		fetched = true;
		syncCondition.notify_all();
	});
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	GPGManager::getAchievementProgress(stat + ulongToString(maximumMilestone - 1), [&](int val)
	{
		std::lock_guard<std::mutex> syncGuard(syncMutex);

		if (maxVal < val)
			maxVal = val;

		fetched = true;
		syncCondition.notify_all();
	});
#endif

	{
		std::unique_lock<std::mutex> syncLock(syncMutex);
		syncCondition.wait(syncLock, [&] { return fetched; });
	}

    printf("Syncing stat %s with value %d\n", stat.c_str(), maxVal);
	UserDefault::getInstance()->setIntegerForKey(stat.c_str(), maxVal);
	AchievementManager::updateStat(stat, maxVal);
#endif
}

void AchievementManager::initialize()
{
	for (const auto &pair : stats)
		if (pair.second.type == StatData::Type::INCR)
			std::thread(syncStat, pair.first).detach();
}

inline void unlockAchievement(std::string achievement)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    GameCenterManager::unlockAchievement(achievement);
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    auto it = achievementMapToGPG.find(achievement);
	if (it != achievementMapToGPG.end() && !it->second.empty())
		GPGManager::unlockAchievement(it->second);
#endif
}

inline void updateAchievementStatus(std::string achievement, int cur, int total)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    GameCenterManager::updateAchievementStatus(achievement, 100.0 * (double)cur/total);
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    auto it = achievementMapToGPG.find(achievement);
	if (it != achievementMapToGPG.end() && !it->second.empty())
		GPGManager::updateAchievementStatus(it->second, cur);
#endif
}

void AchievementManager::updateStat(std::string stat, int value)
{
	auto it = stats.find(stat);
	if (it == stats.end()) return;

	int i = 0;
	for (auto milestone : it->second.milestones)
	{
		if (it->second.type == StatData::Type::FULL && value < milestone) break;

		if (it->second.type == StatData::Type::FULL)
			unlockAchievement(stat + ulongToString(i));
		else updateAchievementStatus(stat + ulongToString(i), MIN(value, milestone), milestone);

		i++;
	}
}

void AchievementManager::getStatData(std::string stat, std::function<void(int)> handler)
{

}

void AchievementManager::increaseStat(std::string stat, int value)
{
	int oldVal = UserDefault::getInstance()->getIntegerForKey(stat.c_str());
	UserDefault::getInstance()->setIntegerForKey(stat.c_str(), MIN(oldVal + value, 40000000));
	updateStat(stat, oldVal + value);
}
