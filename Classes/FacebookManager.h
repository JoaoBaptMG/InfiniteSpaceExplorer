//
//  FacebookManager.h
//  SpaceExplorer
//
//  Created by João Baptista on 06/07/15.
//
//

#ifndef __SpaceExplorer__FacebookManager__
#define __SpaceExplorer__FacebookManager__

#include "cocos2d.h"
#include "ScoreManager.h"

#define PARAMS_EMPTY (std::unordered_map<std::string, std::string>())

enum class HTTPMethod { GET, POST, DELETE };

namespace FacebookManager
{
    enum class PermissionState { UNKNOWN, ERROR, DECLINED, ACCEPTED };
    
    void initialize();
    
    bool isAccessTokenValid();
    std::string getUserID();
    std::string getUserName();
    void requestReadPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest = false);
    void requestPublishPermissions(std::function<void(FacebookManager::PermissionState, std::string)> callback, bool rerequest = false);
    void logOut();
    
    bool hasPermission(std::string permission);
    void graphRequest(std::string path, const std::unordered_map<std::string, std::string> &parameters,
                      HTTPMethod method, std::function<void(cocos2d::Value&&, std::string)> callback);
    
    void loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler);
    void loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
                               long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler, bool loadPhotos = true);
    void reportScore(int64_t score);
    
    void shareScore(int64_t score);
}

#endif /* defined(__SpaceExplorer__FacebookManager__) */
