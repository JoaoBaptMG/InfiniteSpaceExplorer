//
//  GameCenterManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 30/07/15.
//
//

#include "cocos2d.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif

#include "GameCenterManager.h"
#include "../proj.ios_mac/ios/AppController.h"

#import <GameKit/GameKit.h>

#define LEADERBOARD_ID @"gameScore"

using namespace GameCenterManager;
using namespace cocos2d;

static bool gameCenterActive = false;
static bool awaitingViewController = false;

static GKLeaderboard *globalLeaderboard;
static ScoreManager::ScoreData cachedPlayerData(-1, "", 0, true);
static bool cachedPlayerDataDirty = true;

static std::unordered_map<std::string,GKAchievement*> achievementCache;
static std::mutex achievementCacheMutex;
static std::condition_variable achievementCacheCondition;
static std::once_flag achievementLoading;
static std::atomic_bool achievementsLoaded(false);

inline uint64_t packContext(ScoreManager::AdditionalContext context)
{
    int32_t packedTime = MAX(context.time, 0xFFFFF);
    int32_t packedMultiplier = MAX(context.maxMultiplier, 0x3FF);
    int32_t packedShip = MAX(context.shipUsed, 3);
    
    return uint64_t(packedTime | (packedMultiplier << 20) | (packedShip << 30));
}

inline ScoreManager::AdditionalContext unpackContext(uint64_t ctx)
{
    int32_t unpackedTime = ctx & 0xFFFFF;
    int32_t unpackedMultiplier = (ctx >> 20) & 0x3FF;
    int32_t unpackedShip = (ctx >> 30) & 3;
    
    return { unpackedTime, unpackedMultiplier, unpackedShip };
}

inline void initialize()
{
    globalLeaderboard = [[GKLeaderboard alloc] init];
    globalLeaderboard.identifier = LEADERBOARD_ID;
    
    cachedPlayerDataDirty = true;
}

void GameCenterManager::authenticate(std::function<void()> success)
{
    gameCenterActive = false;
    [GKLocalPlayer localPlayer].authenticateHandler = ^ (UIViewController *viewController, NSError *error)
    {
        if (viewController != nil)
        {
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("FreezeForSocialManagers");
            UIViewController *rootController = [UIApplication sharedApplication].keyWindow.rootViewController;
            [rootController presentViewController:viewController animated:YES completion:nil];
            awaitingViewController = true;
        }
        else if ([GKLocalPlayer localPlayer].isAuthenticated)
        {
            initialize();
            gameCenterActive = true;
            success();
        }
        else gameCenterActive = false;
        
        if (awaitingViewController)
        {
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("UnfreezeForSocialManagers");
            awaitingViewController = false;
        }
    };
}

void GameCenterManager::loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler)
{
    if (gameCenterActive)
    {
        if (cachedPlayerDataDirty)
        {
            GKLeaderboard *playerLeaderboard = [[GKLeaderboard alloc] initWithPlayers:@[[GKLocalPlayer localPlayer].playerID]];
            playerLeaderboard.identifier = LEADERBOARD_ID;
            playerLeaderboard.timeScope = GKLeaderboardTimeScopeAllTime;
            
            [playerLeaderboard loadScoresWithCompletionHandler:^ (NSArray *scores, NSError *error)
             {
                 if (scores.count > 0)
                 {
                     GKScore *playerScore = scores[0];
                     cachedPlayerData.index = playerScore.rank;
                     cachedPlayerData.name.assign(playerScore.player.displayName.UTF8String);
                     cachedPlayerData.score = playerScore.value;
                     cachedPlayerData.context = unpackContext(playerScore.context);
                     
                     handler(cachedPlayerData);
                 }
             }];
            
            cachedPlayerDataDirty = false;
        }
        else handler(cachedPlayerData);
    }
}

void GameCenterManager::loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
                                              long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler, bool loadPhotos)
{
    if (gameCenterActive)
    {
        switch (timeConstraint)
        {
            case ScoreManager::TimeConstraint::ALL: globalLeaderboard.timeScope = GKLeaderboardTimeScopeAllTime; break;
            case ScoreManager::TimeConstraint::WEEKLY: globalLeaderboard.timeScope = GKLeaderboardTimeScopeWeek; break;
            case ScoreManager::TimeConstraint::DAILY: globalLeaderboard.timeScope = GKLeaderboardTimeScopeToday; break;
            default: break;
        }
        
        switch (socialConstraint)
        {
            case ScoreManager::SocialConstraint::GLOBAL: globalLeaderboard.playerScope = GKLeaderboardPlayerScopeGlobal; break;
            case ScoreManager::SocialConstraint::FRIENDS: globalLeaderboard.playerScope = GKLeaderboardPlayerScopeFriendsOnly; break;
            default: break;
        }
        
            globalLeaderboard.range = NSMakeRange(first, MIN(last-first+1, 100));
            [globalLeaderboard loadScoresWithCompletionHandler:^(NSArray *scores, NSError *error)
            {
                if (scores != nil)
                {
                    std::vector<ScoreManager::ScoreData> vector;
                    vector.reserve(scores.count);
                    
                    for (GKScore *score in scores)
                    {
                        ScoreManager::ScoreData data(score.rank, std::string(score.player.displayName.UTF8String), score.value,
                                                     [score.playerID isEqualToString:[GKLocalPlayer localPlayer].playerID]);
                        data.context = unpackContext(score.context);
                        
                        std::string textureKey = "Photo" + std::string(score.playerID.UTF8String);
                        data.textureKey = textureKey;
                        
                        if (loadPhotos)
                        {
                            if (Director::getInstance()->getTextureCache()->getTextureForKey(textureKey) == nullptr)
                                [score.player loadPhotoForSize:GKPhotoSizeNormal withCompletionHandler:^(UIImage *photo, NSError *error)
                                {
                                    if (photo != nil)
                                    {
                                        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
                                        {
                                            NSData *png = UIImagePNGRepresentation(photo);
                                            if (png != nil)
                                            {
                                                Image *img = new(std::nothrow) Image();
                                                if (img != nullptr)
                                                {
                                                    img->initWithImageData(reinterpret_cast<const unsigned char*>(png.bytes), png.length);
                                                    
                                                    Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]
                                                    {
                                                        Texture2D* texture = Director::getInstance()->getTextureCache()->addImage(img, textureKey);
                                                        img->release();
                                                        
                                                        auto texPtr = static_cast<void*>(const_cast<Texture2D**>(&texture));
                                                        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("TextureArrived." + textureKey, texPtr);
                                                    });
                                                }
                                                else Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]
                                                {
                                                    Texture2D* texture = nullptr;
                                                    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("TextureArrived." + textureKey, &texture);
                                                });
                                            }
                                            else Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]
                                            {
                                                Texture2D* texture = nullptr;
                                                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("TextureArrived." + textureKey, &texture);
                                            });
                                        });
                                    }
                                    else Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]
                                    {
                                        Texture2D* texture = nullptr;
                                        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("TextureArrived." + textureKey, &texture);
                                    });
                                }];
                        }
                        
                        vector.push_back(std::move(data));
                    }
                    
                    auto val = vector[0].index;
                    handler(val, std::move(vector), "");
                    
                    cachedPlayerDataDirty = true;
                }
                else
                {
                    std::string errorString = "";
                    if (error != nil)
                    {
                        if ([error.domain isEqualToString:NSURLErrorDomain])
                        {
                            if (error.code == NSURLErrorNotConnectedToInternet)
                                errorString = "Your device is not connected to the internet!";
                            else
                                errorString = error.localizedDescription.UTF8String;
                        }
                        else errorString = error.localizedDescription.UTF8String;
                    }
                    handler(-1, std::vector<ScoreManager::ScoreData>(), errorString);
                }
            }];
    }
    else handler(-1, std::vector<ScoreManager::ScoreData>(), "Game Center is not available!");
}

void GameCenterManager::reportScore(int64_t score, ScoreManager::AdditionalContext context)
{
    GKScore *scoreObj = [[GKScore alloc] initWithLeaderboardIdentifier:LEADERBOARD_ID player:[GKLocalPlayer localPlayer]];
    scoreObj.value = score;
    scoreObj.context = packContext(context);
    
    [GKScore reportScores:@[scoreObj] withCompletionHandler:nil];
    
    cachedPlayerDataDirty = true;
}

void GameCenterManager::unlockAchievement(std::string achId)
{
	updateAchievementStatus(achId, 100);
}

void loadAchievementCache()
{
    std::unique_lock<std::mutex> achievementCacheLock(achievementCacheMutex);
    
    [GKAchievement loadAchievementsWithCompletionHandler: ^(NSArray *achievements, NSError *error)
    {
        if(error == nil)
        {
            std::unordered_map<std::string,GKAchievement*> tempCache;
            
            for (GKAchievement* achievement in achievements)
                tempCache.emplace(achievement.identifier.UTF8String, achievement);
            
            {
                std::lock_guard<std::mutex> achievementCacheGuard(achievementCacheMutex);
                achievementCache = std::move(tempCache);
                achievementsLoaded.store(true);
                achievementCacheCondition.notify_all();
            }
        }
    }];
    
    achievementCacheCondition.wait(achievementCacheLock, [] { return !achievementsLoaded; });
}

void GameCenterManager::updateAchievementStatus(std::string achId, double percent)
{
    if (percent == 0) return;
    percent = MIN(percent, 100);

    std::call_once(achievementLoading, loadAchievementCache);
    std::lock_guard<std::mutex> achievementCacheGuard(achievementCacheMutex);
    
    auto it = achievementCache.find(achId);
    if (it != achievementCache.end())
    {
        GKAchievement* achievement = it->second;
        if (achievement.percentComplete >= percent) achievement = nil;
        else achievement.percentComplete = percent;
    }
    else
    {
        NSString* identifier = [NSString stringWithCString:achId.c_str() encoding:NSUTF8StringEncoding];
        GKAchievement* achievement = [[GKAchievement alloc] initWithIdentifier:identifier];
        achievement.percentComplete = percent;
        it = achievementCache.emplace(achievement.identifier.UTF8String, achievement).first;
    }
    
    GKAchievement* achievement = it->second;
    if (achievement != nil) [GKAchievement reportAchievements:@[achievement] withCompletionHandler:^(NSError*) {}];
}

void GameCenterManager::getAchievementProgress(std::string achId, std::function<void(double)> handler)
{
    std::call_once(achievementLoading, loadAchievementCache);
    
    GKAchievement* achievement = nil;
    {
        std::lock_guard<std::mutex> achievementCacheGuard(achievementCacheMutex);
        auto it = achievementCache.find(achId);
        if (it != achievementCache.end()) achievement = it->second;
    }
    
    if (achievement != nil) handler(achievement.percentComplete);
    else handler(0);
}

void GameCenterManager::presentWidget()
{
    GKGameCenterViewController *controller = [[GKGameCenterViewController alloc] init];
    if (controller != nil)
    {
        controller.gameCenterDelegate = (AppController*)[UIApplication sharedApplication].delegate;
        [[UIApplication sharedApplication].keyWindow.rootViewController presentViewController:controller animated:true completion:nil];
    }
}

#endif
