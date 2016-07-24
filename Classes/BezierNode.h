//
//  BezierNode.h
//  SpaceExplorer
//
//  Created by João Baptista on 16/03/15.
//
//

#ifndef __SpaceExplorer__BezierNode__
#define __SpaceExplorer__BezierNode__

#include "cocos2d.h"

class BezierNode : public cocos2d::Node
{
    cocos2d::V3F_C4B_T2F vertices[6];
    float bezierWidth;
    cocos2d::Vec2 points[3];
    cocos2d::Vec2 dTdx, dTdy;
    cocos2d::TrianglesCommand trianglesCommand;
    cocos2d::TrianglesCommand::Triangles triangles;
    cocos2d::CustomCommand setUniformsCommand;
    cocos2d::Rect boundingBox;
    
    bool init(cocos2d::Color3B color, float width);
    
public:
    static BezierNode *create(cocos2d::Color3B color, float width);
    
    inline void setBezierWidth(float width);
    inline float getBezierWidth() { return bezierWidth; }
    
    void reinputCurve(cocos2d::Vec2 point0, cocos2d::Vec2 point1, cocos2d::Vec2 point2);
    
    virtual cocos2d::Rect getBoundingBox() const override;
    virtual void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags) override;
};

#endif /* defined(__SpaceExplorer__BezierNode__) */
