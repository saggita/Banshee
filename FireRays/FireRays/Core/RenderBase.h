//
//  RenderBase.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef RENDERBASE_H
#define RENDERBASE_H

#include "Common.h"
#include "CommonTypes.h"

class SceneBase;
class CameraBase;
class TextureBase;

/// Main interface for the renderer
/// The renderer queries scene data through
/// SceneBase interface from scene_ member
/// and camera data through CameraBase interface
/// from camera_ member
class RenderBase
{
public:
    /// Abstract interafce
    RenderBase();
	virtual						~RenderBase() = 0;
    
    /// Init renderer for outputting into width x height target
	virtual void				Init(unsigned width, unsigned height) = 0;
    
    /// Commit camera and scene changes if any
	virtual void				Commit() = 0;
    
    /// Render scene
	virtual void				Render() = 0;
    
    /// Get output texture
	virtual unsigned			GetOutputTexture() const = 0;
    
    /// Stop accumulating samples and clear render target
    /// (required if camera, scene or lights are changed)
	virtual void				FlushFrame() = 0;
    
    /// This part has default implementation
    virtual void                AttachTexture(std::string const& name, std::shared_ptr<TextureBase> texture);
    virtual void                DetachTexture(std::string const& name);
	
    /// Concrete methods to set and get scene and camera
	void						SetScene(std::shared_ptr<SceneBase>  scene);
	void						SetCamera(std::shared_ptr<CameraBase> camera);
	
	std::shared_ptr<SceneBase>	GetScene() const;
	std::shared_ptr<CameraBase>	GetCamera() const;
    
protected:
    /// Texture storage
    typedef     std::map<std::string, std::shared_ptr<TextureBase> > TextureMap;
    
    TextureMap::const_iterator TexturesCBegin();
    TextureMap::const_iterator TexturesCEnd();
    
    /// Have textures been changed since last commit?
    bool TexturesDirty() const;
    /// Reset dirty flag
    void ResetTexturesDirty();

private:
	RenderBase& operator = (RenderBase const& other);
	
    /// Scene and camera
	std::shared_ptr<SceneBase>	scene_;
	std::shared_ptr<CameraBase>	camera_;

    /// Textures storage
    bool        texturesDirty_;
    TextureMap  textures_;
};

inline RenderBase::RenderBase()
	: texturesDirty_(false)
{
}

inline RenderBase::~RenderBase() 
{
}

inline void RenderBase::SetScene(std::shared_ptr<SceneBase> scene)
{
	scene_ = scene;
}

inline void RenderBase::SetCamera(std::shared_ptr<CameraBase> camera)
{
	camera_ = camera;
}

inline std::shared_ptr<SceneBase>  RenderBase::GetScene() const
{
	return scene_;
}

inline std::shared_ptr<CameraBase> RenderBase::GetCamera() const
{
	return camera_;
}

inline RenderBase::TextureMap::const_iterator RenderBase::TexturesCBegin()
{
    return textures_.cbegin();
}

inline RenderBase::TextureMap::const_iterator RenderBase::TexturesCEnd()
{
    return textures_.cend();
}

inline bool RenderBase::TexturesDirty() const
{
	return texturesDirty_;
}

inline void RenderBase::ResetTexturesDirty()
{
	texturesDirty_ = false;
}

#endif // RENDERBASE_H
