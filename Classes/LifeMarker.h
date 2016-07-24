//
//  LifeMarker.h
//  SpaceExplorer
//
//  Created by João Baptista on 13/03/15.
//
//

#ifndef __SpaceExplorer__LifeMarker__
#define __SpaceExplorer__LifeMarker__

#include "cocos2d.h"

class LifeMarkers : public cocos2d::Node
{
    cocos2d::Sprite *lifeIndicatorSprite;
    cocos2d::Label *lifeText;
    cocos2d::EventListenerCustom *listener;
    
    float targetLives, currentLives;
    
    void lifeUpdate(cocos2d::EventCustom *event);
    bool init(const cocos2d::Size &screenSize);
    
public:
    virtual void onEnterTransitionDidFinish() override;
    virtual void onEnter() override;
    virtual void onExitTransitionDidStart() override;
    
    virtual void update(float delta) override;
    virtual void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4& transform, uint32_t flags) override;
    
    virtual ~LifeMarkers();
    
    static LifeMarkers *create(const cocos2d::Size &screenSize);
};

#endif /* defined(__SpaceExplorer__LifeMarker__) */
