//
//  ResultNode.h
//  SpaceExplorer
//
//  Created by João Baptista on 21/04/15.
//
//

#ifndef __SpaceExplorer__ResultNode__
#define __SpaceExplorer__ResultNode__

#include "cocos2d.h"

class ResultNode : public cocos2d::Node
{
    bool repushing;
    
    bool init(const cocos2d::Size &screenSize);
    
    void sequence();
    void repushScene();
    void popToMain();
    
    void presentStatusLabel(float delay);
    
    void fbAction();
    void fbShareScore();
    
public:
    static ResultNode *create(const cocos2d::Size &screenSize);
};

#endif /* defined(__SpaceExplorer__ResultNode__) */
