//
//  ExampleScoreManager.hpp
//  SpaceExplorer
//
//  Created by João Baptista on 17/09/15.
//
//

#ifndef ExampleScoreManager_hpp
#define ExampleScoreManager_hpp

#include "ScoreManager.h"

namespace ExampleScoreManager
{
    void loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler);
    void loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
                               long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler);
    void reportScore(int64_t score);
};

#endif /* ExampleScoreManager_hpp */
