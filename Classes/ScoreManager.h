//
//  ScoreManager.h
//  SpaceExplorer
//
//  Created by João Baptista on 03/05/15.
//
//

#ifndef __SpaceExplorer__ScoreManager__
#define __SpaceExplorer__ScoreManager__

#include "cocos2d.h"

#include <string>
#include <functional>
#include <vector>

namespace ScoreManager
{
    enum class TimeConstraint { DAILY = 0, WEEKLY, ALL, NUMBER_OF_TIME_CONSTRAINTS };
    enum class SocialConstraint { FRIENDS = 0, GLOBAL, NUMBER_OF_SOCIAL_CONSTRAINTS };
    enum class Source
    {
        //EXAMPLE,
        PLATFORM_SPECIFIC,
        FACEBOOK,
        NUMBER_OF_SOURCES
    };
    
    struct AdditionalContext
    {
        int32_t time, maxMultiplier, shipUsed;
        bool isValid() const { return shipUsed != 0; }
    };

    struct ScoreData
    {
        long index;
        std::string name;
        std::string textureKey;
        int64_t score;
        bool isPlayer;
        AdditionalContext context;
        
        inline ScoreData() : ScoreData(0, "", 0) {}
        inline ScoreData(long index, std::string name, int64_t score, bool isPlayer = false) :
        index(index), name(name), score(score), textureKey(""), isPlayer(isPlayer), context() {}
    };
    
    void init();
    void loadPlayerCurrentScore(std::function<void(const ScoreData&)> handler);
    void loadHighscoresOnRange(long first, long last, std::function<void(long, std::vector<ScoreData>&&, std::string)> handler);
    void reportScore();
    
    void updateScoreTrackingArray();
    ScoreData getNextTrackedScore(int64_t score);
    bool trackedScoresReady();
    
    extern TimeConstraint currentTimeConstraint;
    extern SocialConstraint currentSocialConstraint;
    extern Source currentSource;
}

#endif /* defined(__SpaceExplorer__ScoreManager__) */
