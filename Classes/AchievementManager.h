//
//  AchievementManager.h
//  SpaceExplorer
//
//  Created by João Baptista on 17/08/16.
//
//

#ifndef __SpaceExplorer__AchievementManager__
#define __SpaceExplorer__AchievementManager__

#include <string>
#include <functional>

namespace AchievementManager
{
	void initialize();
	void updateStat(std::string stat, int value);

	void getStatData(std::string stat, std::function<void(int)> handler);
	void increaseStat(std::string stat, int value);
}

#endif /* defined(__SpaceExplorer__AchievementManager__) */
