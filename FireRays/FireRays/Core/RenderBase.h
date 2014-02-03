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

class RenderBase
{
public:
    RenderBase();
	virtual						~RenderBase() = 0;
	virtual void				Init(unsigned width, unsigned height) = 0;
	virtual void				Commit() = 0;
	virtual void				Render() = 0;
	virtual unsigned			GetOutputTexture() const = 0;
	virtual void				FlushFrame() = 0;
    virtual void                AttachTexture(std::string const& name, std::shared_ptr<TextureBase> texture);
    virtual void                DetachTexture(std::string const& name);
	
	void						SetScene(std::shared_ptr<SceneBase>  scene);
	void						SetCamera(std::shared_ptr<CameraBase> camera);
	
	std::shared_ptr<SceneBase>	GetScene() const;
	std::shared_ptr<CameraBase>	GetCamera() const;
    
protected:
    typedef     std::map<std::string, std::shared_ptr<TextureBase> > TextureMap;
    
    TextureMap::const_iterator textures_cbegin();
    TextureMap::const_iterator textures_cend();
    
    bool is_textures_dirty() const;
    void reset_textures_dirty();

private:
	RenderBase& operator = (RenderBase const& other);
	
	std::shared_ptr<SceneBase>	scene_;
	std::shared_ptr<CameraBase>	camera_;

    bool        textures_dirty_;
    TextureMap  textures_;
};

inline RenderBase::RenderBase()
: textures_dirty_(false)
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

inline RenderBase::TextureMap::const_iterator RenderBase::textures_cbegin()
{
    return textures_.cbegin();
}

inline RenderBase::TextureMap::const_iterator RenderBase::textures_cend()
{
    return textures_.cend();
}

inline bool RenderBase::is_textures_dirty() const
{
    return textures_dirty_;
}

inline void RenderBase::reset_textures_dirty()
{
    textures_dirty_ = false;
}

#endif // RENDERBASE_H
