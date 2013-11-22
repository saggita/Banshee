//
//  render_base.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_render_base_h
#define BVHOQ_render_base_h

#include <memory>
#include "common_types.h"
#include "scene_base.h"

class scene_base;
class camera_base;

class render_base
{
public:
	virtual ~render_base() = 0;
	
	void set_scene(std::shared_ptr<scene_base>  scene);
	void set_camera(std::shared_ptr<camera_base> camera);
	
	std::shared_ptr<scene_base>  scene() const;
	std::shared_ptr<camera_base> camera() const;
	
	virtual void init(unsigned width, unsigned height) = 0;
	virtual void commit() = 0;
	virtual void render_and_cull(matrix4x4 const& mvp, std::vector<scene_base::mesh_desc> const& meshes) = 0;
	virtual unsigned output_texture() const = 0;

private:
	render_base& operator = (render_base const& other);
	
	std::shared_ptr<scene_base>  scene_;
	std::shared_ptr<camera_base> camera_;
};

inline render_base::~render_base() {}

inline void render_base::set_scene(std::shared_ptr<scene_base> scene)
{
	scene_ = scene;
}

inline void render_base::set_camera(std::shared_ptr<camera_base> camera)
{
	camera_ = camera;
}

inline std::shared_ptr<scene_base>  render_base::scene() const
{
	return scene_;
}

inline std::shared_ptr<camera_base> render_base::camera() const
{
	return camera_;
}

#endif
