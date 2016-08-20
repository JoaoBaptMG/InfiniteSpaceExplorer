//
//  GPGManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 24/07/16.
//
//

#include "GPGManager.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#ifndef __OBJC__
#error This file must be compiled as an Objective-C++ source file!
#endif
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include <jni.h>
#include <string>
#include "platform/android/jni/JniHelper.h"
#endif

#include <gpg/gpg.h>
#include "DownloadPicture.h"

using namespace cocos2d;

static std::unique_ptr<gpg::GameServices> gameServices;
static bool gpgActive = false;

static ScoreManager::ScoreData cachedPlayerData;
static std::string playerId, playerName;
bool playerDataGot = false;
static std::mutex playerDataMutex;
static std::condition_variable playerDataCondition;

static GPGManager::SignStatus signStatus = GPGManager::SignStatus::NOT_SIGNED;

std::string getPlayerId()
{
	std::unique_lock<std::mutex> lock(playerDataMutex);
	playerDataCondition.wait(lock, [=] { return playerDataGot; });
	return playerId;
}

std::string getPlayerName()
{
	std::unique_lock<std::mutex> lock(playerDataMutex);
	playerDataCondition.wait(lock, [=] { return playerDataGot; });
	return playerName;
}

#define LEADERBOARD_ID "CgkIucWamdkeEAIQAQ"

void GPGManager::initialize()
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
	gpg::IosPlatformConfiguration config;
	config.SetClientID("T1054735770297-nec7sp5kg6d6ibvavmevlk6vs2i813v3.apps.googleusercontent.com");
	config.SetOptionalViewControllerForPopups([[UIApplication sharedApplication].keyWindow.rootViewController);
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	JniMethodInfo getActivityInfo;
	auto loaded = JniHelper::getStaticMethodInfo(getActivityInfo, "org/cocos2dx/cpp/AppActivity", "getActivity", "()Landroid/app/Activity;");
	CC_ASSERT(loaded);

	jobject activity = getActivityInfo.env->CallStaticObjectMethod(getActivityInfo.classID, getActivityInfo.methodID);
	getActivityInfo.env->DeleteLocalRef(getActivityInfo.classID);

	gpg::AndroidPlatformConfiguration config;
	config.SetActivity(activity);
#endif

	auto onAuthStarted = [](gpg::AuthOperation op)
	{
		signStatus = GPGManager::SignStatus::SIGNING;

		if (op == gpg::AuthOperation::SIGN_OUT)
		{
			log("Sign out started!");

			gpgActive = false;
			playerId = playerName = "";
		}
		else log("Sign in started!");

		Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("GPGStatusUpdated");
	};

	auto onAuthFinished = [=](gpg::AuthOperation op, gpg::AuthStatus status)
	{
		if (op == gpg::AuthOperation::SIGN_IN)
		{
			if (gpg::IsSuccess(status))
			{
				log("Sign in successful!");

				signStatus = GPGManager::SignStatus::SIGNED;
				gameServices->Players().FetchSelf([=](const gpg::PlayerManager::FetchSelfResponse &response)
				{
					std::lock_guard<std::mutex> lockGuard(playerDataMutex);

					if (gpg::IsSuccess(response.status))
					{
						playerId = response.data.Id();
						playerName = response.data.Name();
					}

					playerDataGot = true;
					playerDataCondition.notify_all();
				});
			}
			else 
			{
				log("Sign in failed!"); 
				signStatus = GPGManager::SignStatus::NOT_SIGNED;
			}
		}
		else
		{
			signStatus = GPGManager::SignStatus::NOT_SIGNED;
			log("Sign out successful!");
		}

		Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("GPGStatusUpdated");
	};

	gameServices = gpg::GameServices::Builder()
		.SetOnAuthActionStarted(onAuthStarted)
		.SetOnAuthActionFinished(onAuthFinished)
		.SetOnLog([](gpg::LogLevel level, const std::string &error) { log("%s", error.c_str()); }, gpg::LogLevel::VERBOSE)
		.Create(config);

	if (!gameServices) signStatus = GPGManager::SignStatus::PLATFORM_UNAVAILABLE;

	log("GPG Status: %d", (int)signStatus);
}

bool GPGManager::isPlatformAvailable()
{
	return (bool)gameServices;
}

GPGManager::SignStatus GPGManager::getSignStatus()
{
	return signStatus;
}

void GPGManager::signIn()
{
	if (gameServices) gameServices->StartAuthorizationUI();
}

void GPGManager::signOut()
{
	if (gameServices) gameServices->SignOut();
}

void GPGManager::loadPlayerCurrentScore(std::function<void(const ScoreManager::ScoreData&)> handler)
{
	if (signStatus != GPGManager::SignStatus::SIGNED) return;

	gameServices->Leaderboards().FetchScoreSummary(LEADERBOARD_ID, gpg::LeaderboardTimeSpan::ALL_TIME,
		gpg::LeaderboardCollection::SOCIAL, [=](const gpg::LeaderboardManager::FetchScoreSummaryResponse& response)
	{
		if (IsSuccess(response.status))
		{
			auto score = response.data.CurrentPlayerScore();
			cachedPlayerData.index = score.Rank();
			cachedPlayerData.score = score.Value();
			cachedPlayerData.name = getPlayerName();
		}

		handler(cachedPlayerData);
	});
}

static constexpr long PageSize = 25;

static long cachedPrevCurrentFirst = -1, cachedNextCurrentFirst = -1;
static gpg::ScorePage::ScorePageToken cachedPrevToken, cachedNextToken;

struct scoreGatherer
{
	std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler;
	long currentFirst;
	long requestedFirst, requestedLast;
	gpg::ScorePage::ScorePageToken curToken;
	std::deque<ScoreManager::ScoreData> scores;
	bool backward, firstRequest, loadPhotos;

	scoreGatherer(std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler,
		long currentFirst, long requestedFirst, long requestedLast, gpg::ScorePage::ScorePageToken curToken, bool loadPhotos)
		: handler(handler), currentFirst(currentFirst), requestedFirst(requestedFirst), requestedLast(requestedLast),
		  curToken(curToken), loadPhotos(loadPhotos)
	{
		backward = currentFirst > requestedFirst;
		firstRequest = true;
	}

	void requestToken()
	{
		long numberOfEntries = MIN(requestedLast - currentFirst + 1, PageSize);
		numberOfEntries = MAX(numberOfEntries, 0);
		gameServices->Leaderboards().FetchScorePage(curToken, std::ref(*this));
	}

	void operator()(const gpg::LeaderboardManager::FetchScorePageResponse& response)
	{
		if (!gpg::IsSuccess(response.status))
			return finalize(true, "Could not load all scores!");

		auto page = response.data;

		if (firstRequest)
		{
			if (backward)
			{
				cachedNextCurrentFirst = currentFirst + PageSize;
				cachedNextToken = page.NextScorePageToken();
			}
			else
			{
				cachedPrevCurrentFirst = currentFirst - PageSize;
				cachedPrevToken = page.PreviousScorePageToken();
			}
		}

		firstRequest = false;

		auto begin = page.Entries().begin() + MIN(MAX(requestedFirst - currentFirst, 0), page.Entries().size());
		auto end = page.Entries().begin() + MIN(MIN(requestedLast - currentFirst + 1, PageSize), page.Entries().size());

		auto dist = std::distance(begin, end);

		decltype(scores) currentScores;
		currentScores.resize(dist);
		bool error = false;

		{
			std::mutex nameWriterMutex;
			std::condition_variable nameWriterCondition;
			long namesWritten = 0;
			long i = 0;

			for (auto it = begin; it != end; ++it, ++i)
			{
				currentScores[i].score = it->Score().Value();
				currentScores[i].index = it->Score().Rank();

				currentScores[i].isPlayer = it->PlayerId() == playerId;
				
				log("Retrieved score id %ld!", i);

				gameServices->Players().Fetch(it->PlayerId(), [&, i](const gpg::PlayerManager::FetchResponse &response)
				{
					std::string textureKey;

					log("Lock mutex!");
					nameWriterMutex.lock();
					log("Mutex locked!");

					if (gpg::IsSuccess(response.status))
					{
						currentScores[i].name = response.data.Name();
						currentScores[i].textureKey = textureKey = "Avatar" + response.data.Id();;
					}
					else error = true;

					namesWritten++;

					log("Wait condition!");
					nameWriterCondition.notify_all();
					nameWriterMutex.unlock();
					log("Condition notified!");

					if (gpg::IsSuccess(response.status) && loadPhotos && Director::getInstance()->getTextureCache()->getTextureForKey(textureKey) == nullptr)
					{
						downloadPicture(response.data.AvatarUrl(gpg::ImageResolution::HI_RES), textureKey, [=](Texture2D* texture)
						{
							Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("TextureArrived." + textureKey, &texture);
						});
					}
				});
			}

			std::unique_lock<std::mutex> nameWriterLock(nameWriterMutex);
			nameWriterCondition.wait(nameWriterLock, [&, dist] { return namesWritten == dist; });
			log("Successful!");
		}

		if (error)
			return finalize(true, "Could not load all scores!");

		scores.insert(backward ? scores.begin() : scores.end(), currentScores.begin(), currentScores.end());

		if (backward)
		{
			currentFirst -= PageSize;
			curToken = page.PreviousScorePageToken();
			if (curToken.Valid() && currentFirst + PageSize > requestedFirst) requestToken();
			else
			{
				requestedFirst = currentFirst + PageSize;
				return finalize();
			}
		}
		else
		{
			currentFirst += PageSize;
			curToken = page.NextScorePageToken();
			if (curToken.Valid() && currentFirst <= requestedLast) requestToken();
			else return finalize();
		}
	}

	void finalize(bool error = false, std::string errorString = "")
	{
		if (error)
			handler(-1, std::vector<ScoreManager::ScoreData>(), errorString);
		else
		{
			if (backward)
			{
				cachedPrevCurrentFirst = currentFirst;
				cachedPrevToken = curToken;
			}
			else
			{
				cachedNextCurrentFirst = currentFirst;
				cachedNextToken = curToken;
			}

			handler(requestedFirst, std::vector<ScoreManager::ScoreData>(scores.begin(), scores.end()), "");
		}

		delete this;
	}
};

void GPGManager::loadHighscoresOnRange(ScoreManager::SocialConstraint socialConstraint, ScoreManager::TimeConstraint timeConstraint,
	long first, long last, std::function<void(long, std::vector<ScoreManager::ScoreData>&&, std::string)> handler, bool loadPhotos)
{
	if (signStatus == GPGManager::SignStatus::PLATFORM_UNAVAILABLE) return handler(-1, {}, "Google Play Games is not available!");
	if (signStatus != GPGManager::SignStatus::SIGNED) return handler(-1, {}, "You are not signed in!");

	getPlayerId(); // This will load the playerId previously

	gpg::ScorePage::ScorePageToken token;
	long currentFirst = 1;

	if (cachedPrevToken.Valid() && (cachedPrevCurrentFirst - 1) / PageSize == (last - 1) / PageSize)
	{
		token = cachedPrevToken;
		currentFirst = cachedPrevCurrentFirst;
	}
	else if (cachedNextToken.Valid() && (cachedNextCurrentFirst - 1) / PageSize == (first - 1) / PageSize)
	{
		token = cachedNextToken;
		currentFirst = cachedNextCurrentFirst;
	}
	else
	{
		gpg::LeaderboardTimeSpan timeSpan;
		gpg::LeaderboardCollection collection;

		switch (timeConstraint)
		{
			case ScoreManager::TimeConstraint::ALL: timeSpan = gpg::LeaderboardTimeSpan::ALL_TIME; break;
			case ScoreManager::TimeConstraint::WEEKLY: timeSpan = gpg::LeaderboardTimeSpan::WEEKLY; break;
			case ScoreManager::TimeConstraint::DAILY: timeSpan = gpg::LeaderboardTimeSpan::DAILY; break;
			default: break;
		}

		switch (socialConstraint)
		{
			case ScoreManager::SocialConstraint::GLOBAL: collection = gpg::LeaderboardCollection::PUBLIC; break;
			case ScoreManager::SocialConstraint::FRIENDS: collection = gpg::LeaderboardCollection::SOCIAL; break;
			default: break;
		}

		token = gameServices->Leaderboards().ScorePageToken(LEADERBOARD_ID, gpg::LeaderboardStart::TOP_SCORES, timeSpan, collection);
	}

	auto gatherer = new scoreGatherer(handler, currentFirst, first, last, token, loadPhotos);
	gatherer->requestToken();
}

void GPGManager::reportScore(int64_t score)
{
	if (gameServices) gameServices->Leaderboards().SubmitScore(LEADERBOARD_ID, score);
}

void GPGManager::unlockAchievement(std::string id)
{
	if (gameServices) gameServices->Achievements().Unlock(id);
}

void GPGManager::updateAchievementStatus(std::string id, int val)
{
	if (gameServices) gameServices->Achievements().SetStepsAtLeast(id, val);
}

void GPGManager::getAchievementProgress(std::string id, std::function<void(int)> handler)
{
	if (gameServices)
		gameServices->Achievements().Fetch(id, [=](const gpg::AchievementManager::FetchResponse& response)
	{
		if (gpg::IsSuccess(response.status))
			handler(response.data.CurrentSteps());
		else handler(-1);
	});
	else handler(-1);
}

#endif
