//
//  GPGManager.h
//  SpaceExplorer
//
//  Created by João Baptista on 24/07/16.
//
//

#ifndef GPGManager_h
#define GPGManager_h

#include "cocos2d.h"
#include "ScoreManager.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

namespace GPGManager
{
	enum class SignStatus { NOT_SIGNED, SIGNING, SIGNED, PLATFORM_UNAVAILABLE };

	void initialize();

	bool isPlatformAvailable();
	SignStatus getSignStatus();

	void signIn();
	void signOut();

	void loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler);
	void loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
		long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler, bool loadPhotos = true);
	void reportScore(int64_t score);

	void unlockAchievement(std::string id);
	void updateAchievementStatus(std::string id, int val);
	void getAchievementProgress(std::string id, std::function<void(int)> handler);
}

#endif

#endif /* GPGManager_h */
