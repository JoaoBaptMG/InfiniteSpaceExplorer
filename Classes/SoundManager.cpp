//
//  SoundManager.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 24/10/15.
//
//

#include "SoundManager.h"
#include "audio/include/SimpleAudioEngine.h"

using namespace cocos2d;

void SoundManager::play(std::string file)
{
    CocosDenshion::SimpleAudioEngine::getInstance()->setEffectsVolume(UserDefault::getInstance()->getIntegerForKey("SoundVolume") / 100.0f);
    CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(FileUtils::getInstance()->fullPathForFilename(file).c_str());
}

void SoundManager::updateBackgroundVolume()
{
    CocosDenshion::SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(UserDefault::getInstance()->getIntegerForKey("MusicVolume") / 100.0f);
}