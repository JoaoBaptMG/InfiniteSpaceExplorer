//
//  DownloadedPhotoNode.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 30/07/15.
//
//

#include "DownloadedPhotoNode.h"

using namespace cocos2d;

inline static Texture2D *getBlankTexture()
{
    static RefPtr<Texture2D> blank = nullptr;
    if (blank == nullptr)
    {
        int size = ceilf(48*Director::getInstance()->getContentScaleFactor());
        
        unsigned int *tex = new unsigned int[size*size];
        for (int i = 0; i < size*size; i++) tex[i] = 0xFF333333;
        
        blank = new (std::nothrow) Texture2D;
        blank->initWithData(tex, sizeof(unsigned int)*size*size, Texture2D::PixelFormat::RGBA8888, size, size, Size(size, size));
        delete[] tex;
    }
    return blank;
}

bool DownloadedPhotoNode::init()
{
    if (!Sprite::initWithTexture(getBlankTexture())) return false;
    
    setBlendFunc(BlendFunc::ALPHA_NON_PREMULTIPLIED);
    setGLProgramState(GLProgramState::getOrCreateWithGLProgramName("ScoreWidgetProgram"));
    getGLProgramState()->setUniformFloat("size", 48 * _director->getContentScaleFactor());
    
    waitForTextureListener = nullptr;
    isDownloaded = false;
    
    return true;
}

DownloadedPhotoNode::~DownloadedPhotoNode()
{
    if (waitForTextureListener) _eventDispatcher->removeEventListener(waitForTextureListener);
}

void DownloadedPhotoNode::setTextureKey(std::string key)
{
    if (waitForTextureListener) _eventDispatcher->removeEventListener(waitForTextureListener);
    
    waitForTextureListener = nullptr;
    textureKey = key;
    isDownloaded = false;
    
    if (!key.empty())
    {
        Texture2D *texture = _director->getTextureCache()->getTextureForKey(key);
        if (texture)
        {
            setTexture(texture);
            isDownloaded = true;
        }
        else
        {
            setTexture(getBlankTexture());

            waitForTextureListener = _eventDispatcher->addCustomEventListener("TextureArrived." + key, [this] (EventCustom* event)
            {
                Texture2D *texture = *static_cast<Texture2D**>(event->getUserData());
                if (texture != nullptr) setTexture(texture);
                
                _eventDispatcher->removeEventListener(waitForTextureListener);
                waitForTextureListener = nullptr;
            });
        }
    }
    else
    {
        setTexture(getBlankTexture());
        isDownloaded = true;
    }
}