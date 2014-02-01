//
//  RenderBase.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef RENDERBASE_H
#define RENDERBASE_H

#include <memory>
#include "CommonTypes.h"
#include "SceneBase.h"

class SceneBase;
class CameraBase;

class RenderBase
{
public:
	virtual						~RenderBase() = 0;
	virtual void				Init(unsigned width, unsigned height) = 0;
	virtual void				Commit() = 0;
	virtual void				Render() = 0;
	virtual unsigned			GetOutputTexture() const = 0;
	virtual void				FlushFrame() = 0;
	
	void						SetScene(std::shared_ptr<SceneBase>  scene);
	void						SetCamera(std::shared_ptr<CameraBase> camera);
	
	std::shared_ptr<SceneBase>	GetScene() const;
	std::shared_ptr<CameraBase>	GetCamera() const;

private:
	RenderBase& operator = (RenderBase const& other);
	
	std::shared_ptr<SceneBase>	scene_;
	std::shared_ptr<CameraBase>	camera_;
};

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

#endif // RENDERBASE_H
