//
//  ExampleScoreManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 17/09/15.
//
//

#include "ExampleScoreManager.h"
#include "cocos2d.h"
#include <fstream>

ScoreManager::ScoreData samplePlayerData, sampleAllData[50000], sampleFriendData[500];
unsigned long long sampleAllDataSize, sampleFriendDataSize;

static bool inited = false;

void initScoreManager()
{
    if (inited) return;
    
    std::string fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename("SampleScores.cfg");
    std::ifstream input { fullPath };
    
    long i = 0, j = 0;
    while (input)
    {
        std::string name;
        if (!std::getline(input, name)) break;
        
        int64_t score;
        bool isFriend, isPlayer;
        input >> score >> isFriend >> isPlayer;
        
        sampleAllData[i].index = i+1;
        sampleAllData[i].name = name;
        sampleAllData[i].score = score;
        sampleAllData[i].isPlayer = isPlayer;
        
        if (isFriend)
        {
            sampleFriendData[j] = sampleAllData[i];
            sampleFriendData[j].index = j+1;
            j++;
        }
        if (isPlayer) samplePlayerData = sampleAllData[i];
        
        i++;
        
        while (input.peek() == '\n') input.get();
    }
    
    sampleAllDataSize = i;
    sampleFriendDataSize = j;
    
    inited = true;
}

void ExampleScoreManager::loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
                           long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler)
{
    CCLOG("ExampleScoreManager::loadHighscoresOnRange called!");
    initScoreManager();
    
    auto size = socialConstraint == ScoreManager::SocialConstraint::FRIENDS ? sampleFriendDataSize : sampleAllDataSize;
    if (first < 1) first = 1;
    if (last > size) last = size;
    
    std::vector<ScoreManager::ScoreData> result;
    for (long k = first; k <= last; k++)
    {
        if (socialConstraint == ScoreManager::SocialConstraint::FRIENDS)
            result.push_back(sampleFriendData[k-1]);
        else
            result.push_back(sampleAllData[k-1]);
    }
    
    handler(first, std::move(result), "");
}

void ExampleScoreManager::loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler)
{
    initScoreManager();
    
    handler(samplePlayerData);
}

void ExampleScoreManager::reportScore(int64_t score)
{
    initScoreManager();
    
    auto isEqual = [] (const ScoreManager::ScoreData &val) { return val.isPlayer; };
    
    auto allPtr = std::find_if(sampleAllData, sampleAllData+sampleAllDataSize, isEqual);
    auto friendPtr = std::find_if(sampleFriendData, sampleFriendData+sampleFriendDataSize, isEqual);
    
    samplePlayerData.score = allPtr->score = friendPtr->score = score;
    
    while (allPtr - sampleAllData > 1 && (allPtr-1)->score < allPtr->score)
    {
        std::swap(*(allPtr-1), *allPtr);
        std::swap((allPtr-1)->index, allPtr->index);
        allPtr--;
    }
    
    while (friendPtr - sampleFriendData > 1 && (friendPtr-1)->score < friendPtr->score)
    {
        std::swap(*(friendPtr-1), *friendPtr);
        std::swap((friendPtr-1)->index, friendPtr->index);
        friendPtr--;
    }
}