//
//  opencl_render.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__opencl_render__
#define __BVHOQ__opencl_render__

#include <iostream>

//
//  rt_cpu_gold.h
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_opencl_render_h
#define BVHOQ_opencl_render_h

#include <vector>
#include <memory>

#ifdef WIN32
#include <gl/glew.h>
#endif
#include <GLUT/GLUT.h>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#else
#include <CL/cl.h>
#include <CL/cl_gl.h>
#endif

#include "scene_base.h"
#include "camera_base.h"
#include "render_base.h"
#include "common_types.h"
#include "bvh_accel.h"

class opencl_render : public render_base
{
public:
	opencl_render(cl_platform_id platform);
	~opencl_render();
	
	void init(unsigned width, unsigned height);
	void render();
	void commit();

	void cull(matrix4x4 const& mvp, std::vector<bbox> const& bounds);

	GLuint output_texture() const;
	
private:
	struct config
	{
		cl_float3 camera_dir;
		cl_float3 camera_right;
		cl_float3 camera_up;
		cl_float3 camera_pos;
		
		cl_float camera_near_z;
		cl_float camera_pixel_size;
		
		cl_uint output_width;
		cl_uint output_height;
	};
	
	struct __declspec(align(16)) bvh_node
	{
		struct
		{
			cl_float3 pmin;
			cl_float3 pmax;
		} box;
		
		cl_uint prim_start;
		cl_uint right;
		cl_uint parent;
		cl_uint num_prims;
	};

	struct __declspec(align(1)) box
	{
		cl_float3 pmin;
		cl_float3 pmax;
	};

	struct  __declspec(align(1)) draw_command
	{
		cl_uint  count;
		cl_uint  instanceCount;
		cl_uint  firstIndex;
		cl_uint  baseVertex;
		cl_uint  baseInstance;
	};
	
	cl_platform_id platform_;
	cl_device_id   device_;
	cl_device_type device_type_;
	cl_context	 context_;
	cl_command_queue command_queue_;
	cl_program	 rt_program_;
	cl_kernel	  rt_kernel_;
	cl_kernel	  cull_kernel_;
	
	cl_mem		vertices_;
	cl_mem		indices_;
	cl_mem		bvh_;
	cl_mem		config_;
	cl_mem		output_;
	cl_mem		bounds_;
	cl_mem		cull_result_;
	cl_mem		atomic_counter_;
	
	GLuint gl_tex_;
	cl_uint2 output_size_;
	
	config config_data_;

	std::shared_ptr<bvh_accel> accel_;
};

#endif


#endif /* defined(__BVHOQ__opencl_render__) */
