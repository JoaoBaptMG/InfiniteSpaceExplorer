//
//  BezierNode.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 16/03/15.
//
//

#include "BezierNode.h"

using namespace cocos2d;

const Vec2 UVs[3] = { { 0.0, 0.0 }, { 0.5, 0.0 }, { 1.0, 1.0 } };

BezierNode* BezierNode::create(Color3B color, float width)
{
    BezierNode *pRet = new(std::nothrow) BezierNode();
    if (pRet && pRet->init(color, width))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
};

unsigned short indices[] = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5 };

bool BezierNode::init(Color3B color, float width)
{
    if (!Node::init())
        return false;
    
    setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("BezierProgram"));
    setBezierWidth(width);
    setColor(color);
    
    points[0] = points[1] = points[2] = Vec2::ZERO;
    memset(vertices, sizeof(vertices), 0);
    
    triangles.verts = vertices;
    triangles.vertCount = sizeof(vertices)/sizeof(vertices[0]);
    triangles.indices = indices;
    triangles.indexCount = sizeof(indices)/sizeof(indices[0]);
    
    boundingBox.setRect(0, 0, 0, 0);
    
    setScale(1);
    
    setUniformsCommand.func = [this]
    {
        getGLProgramState()->setUniformVec2("dTdx", dTdx);
        getGLProgramState()->setUniformVec2("dTdy", dTdy);
        getGLProgramState()->setUniformFloat("curveWidth", bezierWidth);
    };
    
    dTdx = dTdy = Vec2::ZERO;
    
    return true;
}

void BezierNode::setBezierWidth(float width)
{
    getGLProgramState()->setUniformFloat("curveWidth", bezierWidth = width);
}

Vec3 toVec3(Vec2 v) { return Vec3(v.x, v.y, 0); }
Vec2 toVec2(Vec3 v) { return Vec2(v.x, v.y); }

void BezierNode::reinputCurve(Vec2 p0, Vec2 p1, Vec2 p2)
{
    if (p0 == p1 || p1 == p2) return;
    
    points[0] = p0; points[1] = p1; points[2] = p2;
    
    const auto D = (p0-p2).cross(p1-p2);
    auto dudx = (p1.y-p2.y)/D, dudy = (p2.x-p1.x)/D;
    auto dvdx = (p2.y-p0.y)/D, dvdy = (p0.x-p2.x)/D;
    
    dTdx = dudx * (UVs[0] - UVs[2]) + dvdx * (UVs[1] - UVs[2]);
    dTdy = dudy * (UVs[0] - UVs[2]) + dvdy * (UVs[1] - UVs[2]);
    
    // Extrude the triangle
    auto ccw = (p1-p0).cross(p2-p0) > 0;
    
    const Vec2 *side1[] = { &p0, &p1, &p2 };
    const Vec2 *side2[] = { &p1, &p2, &p0 };
    
    float minX = INFINITY, minY = INFINITY, maxX = -INFINITY, maxY = -INFINITY;
    for (int i = 0; i < 3; i++)
    {
        auto side = (*side2[i] - *side1[i]).getNormalized();
        auto normal = ccw ? side.getRPerp() : side.getPerp();
        auto offset = normal * bezierWidth;
        
        vertices[2*i].vertices = toVec3(*side1[i] + offset);
        vertices[2*i+1].vertices = toVec3(*side2[i] + offset);
        
        minX = MIN(minX, MIN(vertices[2*i].vertices.x, vertices[2*i+1].vertices.x));
        maxX = MAX(maxX, MAX(vertices[2*i].vertices.x, vertices[2*i+1].vertices.x));
        minY = MIN(minY, MIN(vertices[2*i].vertices.y, vertices[2*i+1].vertices.y));
        maxY = MAX(maxY, MAX(vertices[2*i].vertices.y, vertices[2*i+1].vertices.y));
    }
    boundingBox.setRect(minX, minY, maxX-minX, maxY-minY);
    
    for (int i = 0; i < 6; i++)
    {
        auto u = Vec2(dudx, dudy).dot(toVec2(vertices[i].vertices) - p2);
        auto v = Vec2(dvdx, dvdy).dot(toVec2(vertices[i].vertices) - p2);
        
        auto texCoord = UVs[2] + u*(UVs[0]-UVs[2]) + v*(UVs[1]-UVs[2]);
        vertices[i].texCoords = Tex2F(texCoord.x, texCoord.y);
    }
}

Rect BezierNode::getBoundingBox() const
{
    return RectApplyTransform(boundingBox, getNodeToParentTransform());
}

void BezierNode::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
    for (auto &vertex : vertices) vertex.colors = (Color4B)getColor();
    
    setUniformsCommand.init(_globalZOrder, transform, flags);
    trianglesCommand.init(_globalZOrder, 0, getGLProgramState(), BlendFunc::ALPHA_NON_PREMULTIPLIED, triangles, transform, flags);
    
    renderer->addCommand(&setUniformsCommand);
    renderer->addCommand(&trianglesCommand);
}