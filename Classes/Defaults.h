//
//  Defaults.h
//  SpaceExplorer
//
//  Created by João Baptista on 05/03/15.
//
//

#ifndef SpaceExplorer_Defaults_h
#define SpaceExplorer_Defaults_h

#include <string>
#include "cocos2d.h"

inline static cocos2d::Scene *createSceneWithLayer(cocos2d::Layer* layer)
{
    auto scene = cocos2d::Scene::create();
    scene->addChild(layer);
    layer->setName("SceneLayer");
    return scene;
}

#define NUMBER_OF_SHIPS 1

extern unsigned long global_ShipSelect;
extern long global_GameScore;

inline static std::string ulongToString(unsigned long value, int numberOfZeroes = 1)
{
    std::string val(std::max<int>(numberOfZeroes, ceil(log10(value+1))), '0');
    for (auto it = val.rbegin(); it != val.rend(); ++it)
    {
        *it = '0' + (value%10);
        value /= 10;
    }
    return val;
}

inline static std::string longToString(long value, int numberOfZeroes = 1)
{
    if (value >= 0) return ulongToString(value, numberOfZeroes);
    
    std::string val = ulongToString(-value, numberOfZeroes);
    val.insert(0, 1, '-');
    return val;
}

inline static cocos2d::Color3B operator "" _c3(unsigned long long value)
{
    return cocos2d::Color3B((value>>16)&0xFF, (value>>8)&0xFF, value&0xFF);
}

inline static void recursivePause(cocos2d::Node *node)
{
    node->pause();
    for (cocos2d::Node *next : node->getChildren()) recursivePause(next);
}

inline static void recursiveResume(cocos2d::Node *node)
{
    node->resume();
    for (cocos2d::Node *next : node->getChildren()) recursiveResume(next);
}

const cocos2d::Color3B BackgroundColor = 0x3993B0_c3;
constexpr float StandardPlayfieldHeight = 400;

#if CC_TARGET_PLATFORM == CC_PLATFORM_WINRT

namespace cocos2d
{
	std::wstring CC_DLL StringUtf8ToWideChar(const std::string& strUtf8);
	std::string CC_DLL StringWideCharToUtf8(const std::wstring& strWideChar);
}

inline std::string PlatformStringToString(Platform::String^ s)
{
	return cocos2d::StringWideCharToUtf8(std::wstring(s->Data()));
}

inline Platform::String^ PlatformStringFromString(const std::string& s)
{
	std::wstring ws = cocos2d::StringUtf8ToWideChar(s);
	return ref new Platform::String(ws.data(), static_cast<unsigned int>(ws.length()));
}

#endif

#endif
