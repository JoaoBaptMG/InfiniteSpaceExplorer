//
//  SoundManager.hpp
//  SpaceExplorer
//
//  Created by João Baptista on 24/10/15.
//
//

#ifndef SoundManager_hpp
#define SoundManager_hpp

#include "cocos2d.h"

namespace SoundManager
{
    void play(std::string file);
    void updateBackgroundVolume();
    
    inline cocos2d::CallFunc *playAction(std::string file)
    {
        return cocos2d::CallFunc::create(CC_CALLBACK_0(play, file));
    }
}

#endif /* SoundManager_hpp */
