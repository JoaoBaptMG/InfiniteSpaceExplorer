//
//  DownloadedPhotoNode.h
//  SpaceExplorer
//
//  Created by João Baptista on 30/07/15.
//
//

#ifndef __SpaceExplorer__DownloadedPhotoNode__
#define __SpaceExplorer__DownloadedPhotoNode__

#include "cocos2d.h"
#include <functional>

class DownloadedPhotoNode : public cocos2d::Sprite
{
    bool isDownloaded;
    std::string textureKey;
    
    cocos2d::EventListenerCustom *waitForTextureListener;
    
public:
    void setTextureKey(std::string key);
    bool init();
    
    virtual void setTexture(cocos2d::Texture2D* texture) override;
    
    virtual ~DownloadedPhotoNode();
    
    CREATE_FUNC(DownloadedPhotoNode);
};

#endif /* defined(__SpaceExplorer__DownloadedPhotoNode__) */
