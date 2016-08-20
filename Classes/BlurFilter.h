//
//  BlurFilter.h
//  SpaceExplorer
//
//  Created by João Baptista on 02/08/15.
//
//

#ifndef __SpaceExplorer__BlurFilter__
#define __SpaceExplorer__BlurFilter__

#include "cocos2d.h"

void applyBlurFilter(cocos2d::RenderTexture *targetRenderTexture, cocos2d::RenderTexture *helperRenderTexture, cocos2d::RenderTexture *outputRenderTexture, float radius);
void rebuildBlurPrograms();

class BlurNode : public cocos2d::Node
{
    cocos2d::RenderTexture* targetRenderTexture;
    cocos2d::RenderTexture* helperRenderTexture;
    cocos2d::RenderTexture* outputRenderTexture;
    
    cocos2d::RefPtr<cocos2d::Node> targetNode;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
    cocos2d::EventListenerCustom* recreatedListener;
#endif
    
    void createRenderTextures();
    void deleteRenderTextures();
    
public:
    inline const cocos2d::Node *getTargetNode() const { return targetNode.get(); }
    inline void setTargetNode(cocos2d::Node *node) { targetNode = node; }
    
    bool init(cocos2d::Size contentSize, cocos2d::Node *targetNode);
    static BlurNode *create(cocos2d::Size contentSize, cocos2d::Node *targetNode);
    
    virtual ~BlurNode();
    virtual void visit(cocos2d::Renderer *renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) override;
};

#endif /* defined(__SpaceExplorer__BlurFilter__) */
