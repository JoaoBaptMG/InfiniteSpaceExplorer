//
//  GameCenterManager.h
//  SpaceExplorer
//
//  Created by João Baptista on 30/07/15.
//
//

#ifndef __SpaceExplorer__GameCenterManager__
#define __SpaceExplorer__GameCenterManager__

#include "cocos2d.h"
#include "ScoreManager.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#include <functional>

namespace GameCenterManager
{
    void authenticate(std::function<void()> success);
    
    void loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler);
    void loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
                               long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler, bool loadPhotos = true);
    void reportScore(int64_t score, ScoreManager::AdditionalContext context);
    
	void unlockAchievement(std::string achId);
	void updateAchievementStatus(std::string achId, double percent);
	void getAchievementProgress(std::string achId, std::function<void(double)> handler);

    void presentWidget();
}
#endif

#endif /* defined(__SpaceExplorer__GameCenterManager__) */
