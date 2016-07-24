//
//  BlurFilter.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 02/08/15.
//
//

#include "BlurFilter.h"
#include "Defaults.h"
#include <iomanip>
#include <unordered_set>

#define STRINGIFY(x) #x

using namespace cocos2d;

static GLint gl_MaxVaryingVectors = -1;
static std::unordered_set<unsigned long> constructedPrograms;

GLProgram *makeGLProgramForBlurRadius(unsigned long radius, bool rebuild = false)
{
    if (gl_MaxVaryingVectors == -1)
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &gl_MaxVaryingVectors);
    
    CC_ASSERT(radius > 0);
    GLProgram *result = GLProgramCache::getInstance()->getGLProgram("BlurProgramRadius." + ulongToString(radius));
    
    if (rebuild || result == nullptr)
    {
        std::stringstream vertStream, fragStream;
        vertStream << std::fixed << std::setprecision(9);
        fragStream << std::fixed << std::setprecision(9);
        
        float* gaussianWeights = new float[radius+3];
        float weightSum = 0;
        
        float sigma = radius/2;
        float sigmaSq = sigma * sigma;
        float sigmaConst = 0.39894228040143 / sigmaSq;
        
        for (long i = 0; i < radius+3; i++)
        {
            gaussianWeights[i] = sigmaConst * expf(-0.5 * i*i / sigmaSq);
            
            if (i == 0) weightSum += gaussianWeights[i];
            else weightSum += 2*gaussianWeights[i];
        }
        
        for (unsigned long i = 0; i < radius+3; i++)
            gaussianWeights[i] /= weightSum;
        
        if (radius < gl_MaxVaryingVectors-1)
        {
            vertStream << "uniform vec2 offsetSize; attribute vec4 a_position; attribute vec2 a_texCoord;"
            "\n#ifdef GL_ES\nvarying mediump vec2 v_centerBlurTexCoord; "
            "varying mediump vec2 v_offsetBlurTexCoords1[" << radius << "]; varying mediump vec2 v_offsetBlurTexCoords2[" << radius << "];"
            "\n#else\nvarying vec2 v_centerBlurTexCoord; "
            "varying vec2 v_offsetBlurTexCoords1[" << radius << "]; varying vec2 v_offsetBlurTexCoords2[" << radius << "];\n#endif\n"
            "void main()\n{\n    gl_Position = CC_PMatrix * a_position;\n    v_centerBlurTexCoord = a_texCoord;\n";
            
            fragStream << "#ifdef GL_ES\nvarying mediump vec2 v_centerBlurTexCoord; "
            "varying mediump vec2 v_offsetBlurTexCoords1[" << radius << "]; varying mediump vec2 v_offsetBlurTexCoords2[" << radius << "];"
            "\n#else\nvarying vec2 v_centerBlurTexCoord; "
            "varying vec2 v_offsetBlurTexCoords1[" << radius << "]; varying vec2 v_offsetBlurTexCoords2[" << radius << "];\n#endif\nvoid main()\n{\n"
            "    gl_FragColor = texture2D(CC_Texture0, v_centerBlurTexCoord) * " << gaussianWeights[0] << ";\n";
            
            for (unsigned long i = 1; i <= radius; i++)
            {
                vertStream << "    v_offsetBlurTexCoords1[" << (i-1) << "] = a_texCoord - " << i << ".0 * offsetSize;\n"
                              "    v_offsetBlurTexCoords2[" << (i-1) << "] = a_texCoord + " << i << ".0 * offsetSize;\n";
                fragStream << "    gl_FragColor += texture2D(CC_Texture0, v_offsetBlurTexCoords1[" << (i-1) << "]) * " << gaussianWeights[i] << ";\n"
                              "    gl_FragColor += texture2D(CC_Texture0, v_offsetBlurTexCoords2[" << (i-1) << "]) * " << gaussianWeights[i] << ";\n";
            }
            
            vertStream << "}";
            fragStream << "}";
        }
        else
        {
            unsigned long optimized = (radius+1)/2;
            unsigned long varyingNumber = MIN(optimized, gl_MaxVaryingVectors-1);
            
            vertStream << "attribute vec4 a_position; attribute vec2 a_texCoord;"
            "\n#ifdef GL_ES\nuniform highp vec2 offsetSize; varying mediump vec2 v_centerBlurTexCoord; "
            "varying mediump vec2 v_offsetBlurTexCoords1[" << varyingNumber << "]; varying mediump vec2 v_offsetBlurTexCoords2[" << varyingNumber << "];"
            "\n#else\nuniform vec2 offsetSize; varying vec2 v_centerBlurTexCoord; "
            "varying vec2 v_offsetBlurTexCoords1[" << varyingNumber << "]; varying vec2 v_offsetBlurTexCoords2[" << varyingNumber << "];\n#endif\n"
            "void main()\n{\n    gl_Position = CC_PMatrix * a_position;\n    v_centerBlurTexCoord = a_texCoord;\n";
            
            fragStream << "#ifdef GL_ES\n" << (optimized > varyingNumber ? "uniform highp vec2 offsetSize; " : "" ) << "varying mediump vec2 v_centerBlurTexCoord; "
            "varying mediump vec2 v_offsetBlurTexCoords1[" << varyingNumber << "]; varying mediump vec2 v_offsetBlurTexCoords2[" << varyingNumber << "];"
            "\n#else\n" << (optimized > varyingNumber ? "uniform vec2 offsetSize; " : "" ) << "varying vec2 v_centerBlurTexCoord; "
            "varying vec2 v_offsetBlurTexCoords1[" << varyingNumber << "]; varying vec2 v_offsetBlurTexCoords2[" << varyingNumber << "];\n#endif\nvoid main()\n{\n"
            "    gl_FragColor = texture2D(CC_Texture0, v_centerBlurTexCoord) * " << gaussianWeights[0] << ";\n";
            
            for (unsigned long i = 0; i < optimized; i++)
            {
                float first = gaussianWeights[i*2+1];
                float second = gaussianWeights[i*2+2];
                
                float total = first+second;
                float offset = (i*2+1) + second/total;
                
                if (i < varyingNumber)
                {
                    vertStream << "    v_offsetBlurTexCoords1[" << i << "] = a_texCoord - " << offset << " * offsetSize;\n"
                                  "    v_offsetBlurTexCoords2[" << i << "] = a_texCoord + " << offset << " * offsetSize;\n";
                    fragStream << "    gl_FragColor += texture2D(CC_Texture0, v_offsetBlurTexCoords1[" << i << "]) * " << total << ";\n"
                                  "    gl_FragColor += texture2D(CC_Texture0, v_offsetBlurTexCoords2[" << i << "]) * " << total << ";\n";
                }
                else
                    fragStream << "    gl_FragColor += texture2D(CC_Texture0, v_centerBlurTexCoord - " << offset << " * offsetSize) * " << total << ";\n"
                                  "    gl_FragColor += texture2D(CC_Texture0, v_centerBlurTexCoord + " << offset << " * offsetSize) * " << total << ";\n";
            }
            
            vertStream << "}";
            fragStream << "}";
        }
        
        delete[] gaussianWeights;
        
        // If we had to rebuild
        if (result != nullptr)
        {
            result->reset();
            result->initWithByteArrays(vertStream.str().c_str(), fragStream.str().c_str());
            result->link();
            result->updateUniforms();
        }
        else
        {
            result = GLProgram::createWithByteArrays(vertStream.str().c_str(), fragStream.str().c_str());
            GLProgramCache::getInstance()->addGLProgram(result, "BlurProgramRadius." + ulongToString(radius));
            constructedPrograms.insert(radius);
        }
        
    }
    
    return result;
}

void rebuildBlurPrograms()
{
    for (unsigned long radius : constructedPrograms)
        makeGLProgramForBlurRadius(radius, true);
}

static GLProgram* currentProgram = nullptr;
unsigned long currentRadius = -1;

void applyBlurFilter(RenderTexture *target, RenderTexture *helper, RenderTexture *output, float radius)
{
    unsigned long realRadius = radius * Director::getInstance()->getContentScaleFactor();
    
    if (currentRadius != realRadius)
    {
        currentRadius = realRadius;
        currentProgram = makeGLProgramForBlurRadius(currentRadius);
    }
    
    GLProgramState *oldProgramStateTarget = target->getSprite()->getGLProgramState();
    GLProgramState *oldProgramStateHelper = helper->getSprite()->getGLProgramState();
    
    target->getSprite()->getTexture()->setAntiAliasTexParameters();
    helper->getSprite()->getTexture()->setAntiAliasTexParameters();
    
    target->getSprite()->setGLProgramState(GLProgramState::create(currentProgram));
    auto width = target->getSprite()->getContentSize().width * Director::getInstance()->getContentScaleFactor();
    target->getSprite()->getGLProgramState()->setUniformVec2("offsetSize", Vec2(1/width, 0));
    
    helper->getSprite()->setGLProgramState(GLProgramState::create(currentProgram));
    auto height = helper->getSprite()->getContentSize().height * Director::getInstance()->getContentScaleFactor();
    helper->getSprite()->getGLProgramState()->setUniformVec2("offsetSize", Vec2(0, 1/height));
    
    helper->beginWithClear(0, 0, 0, 0);
    target->getSprite()->visit();
    helper->end();
    
    output->beginWithClear(0, 0, 0, 0);
    helper->getSprite()->visit();
    output->end();
    
    target->getSprite()->setGLProgramState(oldProgramStateTarget);
    helper->getSprite()->setGLProgramState(oldProgramStateHelper);
}

BlurNode* BlurNode::create(Size contentSize, Node *targetNode)
{
    BlurNode *pRet = new(std::nothrow) BlurNode();
    if (pRet && pRet->init(contentSize, targetNode))
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
}

bool BlurNode::init(Size contentSize, Node *targetNode)
{
    if (!Node::init()) return false;
    
    addChild(targetNode);
    targetNode->setScale(0.25);
    
    setContentSize(contentSize);
    createRenderTextures();
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WP8
    recreatedListener = _eventDispatcher->addCustomEventListener(EVENT_RENDERER_RECREATED, [this] (EventCustom*) { deleteRenderTextures(); createRenderTextures(); });
#endif
    
    return true;
}

BlurNode::~BlurNode()
{
    deleteRenderTextures();
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WP8
    _eventDispatcher->removeEventListener(recreatedListener);
#endif
}

void BlurNode::createRenderTextures()
{
    CCLOG("Creating render textures!");
    auto size = getContentSize();
    targetRenderTexture = RenderTexture::create(size.width/4, size.height/4, Texture2D::PixelFormat::RGBA8888, GL_DEPTH24_STENCIL8);
    helperRenderTexture = RenderTexture::create(size.width/4, size.height/4, Texture2D::PixelFormat::RGBA8888, GL_DEPTH24_STENCIL8);
    outputRenderTexture = RenderTexture::create(size.width/4, size.height/4, Texture2D::PixelFormat::RGBA8888, GL_DEPTH24_STENCIL8);
    
    targetRenderTexture->retain();
    helperRenderTexture->retain();
    outputRenderTexture->retain();
    
    targetRenderTexture->getSprite()->setAnchorPoint(Vec2::ZERO);
    helperRenderTexture->getSprite()->setAnchorPoint(Vec2::ZERO);
    outputRenderTexture->getSprite()->setAnchorPoint(Vec2::ZERO);
    outputRenderTexture->getSprite()->setScale(4);
    outputRenderTexture->getSprite()->getTexture()->setAntiAliasTexParameters();
}

void BlurNode::deleteRenderTextures()
{
    targetRenderTexture->release();
    helperRenderTexture->release();
    outputRenderTexture->release();
}

void BlurNode::visit(Renderer *renderer, const Mat4& parentTransform, uint32_t parentFlags)
{
    if (!_visible) return;
    
    targetRenderTexture->beginWithClear(0, 0, 0, 0);
    Node::visit(renderer, parentTransform, parentFlags);
    targetRenderTexture->end();
    
    applyBlurFilter(targetRenderTexture, helperRenderTexture, outputRenderTexture, 4);
    
    outputRenderTexture->getSprite()->visit();
}