//
//  DownloadPicture.hpp
//  SpaceExplorer
//
//  Created by João Baptista on 19/09/15.
//
//

#ifndef DownloadPicture_hpp
#define DownloadPicture_hpp

#include "cocos2d.h"

void downloadPicture(std::string path, std::string key, std::function<void(cocos2d::Texture2D*)> callback);

#endif /* DownloadPicture_hpp */
