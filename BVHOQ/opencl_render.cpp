//
//  opencl_render.cpp
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "opencl_render.h"

#include <limits>
#include <chrono>
#include <iostream>
#include <string>
#include <sstream>

#include "render_base.h"
#include "camera_base.h"

#include "utils.h"
#include "bbox.h"

#define CHECK_ERROR(x,m) if((x) != CL_SUCCESS) { std::ostringstream o; o << m <<" error " <<x <<"\n";  throw std::runtime_error(o.str()); }

#define MAX_BOUNDS 1000
#define TILE_SIZE 16

GLuint opencl_render::output_texture() const
{
	return gl_tex_;
}

opencl_render::opencl_render(cl_platform_id platform)
	:platform_(platform)
{
	
}

void opencl_render::init(unsigned width, unsigned height)
{
	output_size_.s[0] = width;
	output_size_.s[1] = height;

	cl_int status = CL_SUCCESS;
	CHECK_ERROR(clGetDeviceIDs(platform_, CL_DEVICE_TYPE_ALL, 1, &device_, nullptr), "GetDeviceIDs failed");

	char device_name[2048];
	clGetDeviceInfo(device_, CL_DEVICE_NAME, 2048, device_name, nullptr);
	std::cout << "Device detected: " << device_name << "\n";

	size_t work_group_size = 0;
	clGetDeviceInfo(device_, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &work_group_size, nullptr);
	std::cout << "Max work group size: " << work_group_size << "\n";
	
	clGetDeviceInfo(device_, CL_DEVICE_TYPE, sizeof(device_type_), &device_type_, nullptr);

	switch (device_type_)
	{
	case CL_DEVICE_TYPE_CPU:
		std::cout << "CPU device\n";
		break;
	case CL_DEVICE_TYPE_GPU:
		std::cout << "GPU device\n";
		break;
	case CL_DEVICE_TYPE_ACCELERATOR:
		std::cout << "Accelerator device\n";
		break;
	default:
		std::cout << "Unknown device\n";
		break;
	}

#ifdef __APPLE__
	CGLContextObj kCGLContext = CGLGetCurrentContext(); // GL Context
	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext); // Share Group
	cl_context_properties props[] =
	{
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties) kCGLShareGroup, CL_CONTEXT_PLATFORM,
		(cl_context_properties) platform_,
		0
	};
#elif WIN32
	cl_context_properties props[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
		CL_WGL_HDC_KHR,
		(cl_context_properties) wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform_, 0
	};
#endif
	
	context_ = clCreateContext(props, 1, &device_, nullptr, nullptr, &status);
	
	CHECK_ERROR(status, "Cannot create OpenCL context");
	
	command_queue_ = clCreateCommandQueue(context_, device_, CL_QUEUE_PROFILING_ENABLE, &status);
	
	CHECK_ERROR(status, "Cannot create command queue");
	
	std::vector<char> source_code;
	load_file_contents("depthmap.cl", source_code);
	
	char* program_source = &source_code[0];
	size_t program_size = source_code.size();
	
	rt_program_ = clCreateProgramWithSource(context_, 1, (const char**)&program_source, &program_size, &status);
	CHECK_ERROR(status, "Cannnot create program from depthmap.cl");
	
	if (clBuildProgram(rt_program_, 1, &device_, nullptr, nullptr, nullptr) < 0)
	{
		std::vector<char> build_log;
		size_t log_size;
		clGetProgramBuildInfo(rt_program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
		
		build_log.resize(log_size);
		clGetProgramBuildInfo(rt_program_, device_, CL_PROGRAM_BUILD_LOG, log_size, &build_log[0], nullptr);
		
		throw std::runtime_error(&build_log[0]);
	};
	
	rt_kernel_ = clCreateKernel(rt_program_, "trace_primary_depth", &status);
	CHECK_ERROR(status, "Cannot create k kernel");

	visibility_check_kernel_ = clCreateKernel(rt_program_, "check_visibility", &status);
	CHECK_ERROR(status, "Cannot create check_visibility kernel");

	command_list_kernel_ = clCreateKernel(rt_program_, "build_command_list", &status);
	CHECK_ERROR(status, "Cannot create build_command_list kernel");
	
	accel_ = bvh_accel::create_from_scene(*scene());

	std::vector<cl_float4> vertices;
	std::for_each(accel_->vertices().cbegin(), accel_->vertices().cend(), [&vertices](vector3 const& v)
				  {
					  cl_float4 val;

					  val.s[0] = v.x();
					  val.s[1] = v.y();
					  val.s[2] = v.z();

					  vertices.push_back(val);
				  });
	
	
	vertices_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * vertices.size(), (void*)&vertices[0], &status);
	CHECK_ERROR(status, "Cannot create vertex buffer");
	
	std::vector<cl_uint4> triangles;
	
	std::for_each(accel_->primitives().cbegin(), accel_->primitives().cend(), [&triangles](bvh_accel::triangle const& t)
				  {
					  cl_uint4 val;

					  val.s[0] = t.i1;
					  val.s[1] = t.i2;
					  val.s[2] = t.i3;

					  triangles.push_back(val);
				  });
	
	indices_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint4) * triangles.size(), (void*)&triangles[0], &status);
	CHECK_ERROR(status, "Cannot create index buffer");
	
	std::vector<bvh_node> nodes(accel_->nodes().size());
	std::transform(accel_->nodes().begin(), accel_->nodes().end(), nodes.begin(),
				   [](bvh_accel::node const& n)->bvh_node
				   {
					   bvh_node nn;

					   nn.box.pmin.s[0] = n.box.min().x();
					   nn.box.pmin.s[1] = n.box.min().y();
					   nn.box.pmin.s[2] = n.box.min().z();
					   nn.box.pmax.s[0] = n.box.max().x();
					   nn.box.pmax.s[1] = n.box.max().y();
					   nn.box.pmax.s[2] = n.box.max().z();

					   nn.right = n.right;
					   nn.prim_start = n.prim_start_index;
					   nn.parent = n.parent;
					   nn.num_prims = n.num_prims;
					   
					   return nn;
				   }
				   );

	bvh_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR , nodes.size() * sizeof(bvh_node), (void*)&nodes[0], &status);
	CHECK_ERROR(status, "Cannot create BVH node buffer");
	
	config_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(config_data_), nullptr, &status);
	CHECK_ERROR(status, "Cannot create parameter buffer");

	bounds_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(cl_float4) * MAX_BOUNDS, nullptr, &status);
	CHECK_ERROR(status, "Cannot create bounds buffer");

	offsets_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(offset) * MAX_BOUNDS, nullptr, &status);
	CHECK_ERROR(status, "Cannot create offsets buffer");

	visibility_buffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_uint) * MAX_BOUNDS, nullptr, &status);
	CHECK_ERROR(status, "Cannot create visibility buffer");

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &gl_tex_);

	glBindTexture(GL_TEXTURE_2D, gl_tex_);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, output_size_.s[0], output_size_.s[1], 0, GL_RGBA, GL_FLOAT, nullptr);

	output_ = clCreateFromGLTexture(context_, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, gl_tex_, &status);
	CHECK_ERROR(status, "Cannot create interop texture");
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenBuffers(1, &gl_buffer_);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gl_buffer_);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(draw_command) * MAX_BOUNDS, nullptr, GL_STATIC_READ);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	cull_result_ = clCreateFromGLBuffer(context_, CL_MEM_WRITE_ONLY, gl_buffer_, &status);
	CHECK_ERROR(status, "Cannot create interop buffer");

#ifdef INDIRECT_PARAMS
	glGenBuffers(1, &gl_atomic_counter_);

	glBindBuffer(GL_PARAMETER_BUFFER_ARB, gl_atomic_counter_);
	glBufferData(GL_PARAMETER_BUFFER_ARB, sizeof(cl_uint), nullptr, GL_STATIC_READ);
	glBindBuffer(GL_PARAMETER_BUFFER_ARB, 0);


	atomic_counter_ = clCreateFromGLBuffer(context_, CL_MEM_WRITE_ONLY, gl_atomic_counter_, &status);
	CHECK_ERROR(status, "Cannot create atomic counter interop buffer");
#else
	atomic_counter_ =  clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &status);
	CHECK_ERROR(status, "Cannot create atomic counter buffer");
#endif

	//cl_image_format fmt = {CL_R, CL_UNSIGNED_INT32};
	//cl_image_desc desc;
	//desc.image_type = CL_MEM_OBJECT_IMAGE1D;
	//desc.image_width = 10 * nodes.size();
	//desc.image_height = 1;
	//desc.image_depth = 1;
	//desc.image_array_size = 1;
	//desc.image_row_pitch = desc.image_slice_pitch = 0;
	//desc.num_mip_levels = 1;
	//desc.num_samples = 1;
	//bvh_texture_ = clCreateImage(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &fmt, &desc, &nodes[0], &status);
	//CHECK_ERROR(status, "Cannot create BVH texture");
}

void opencl_render::commit()
{
	config_data_.output_width = output_size_.s[0];
	config_data_.output_height = output_size_.s[1];
	config_data_.camera_pos.s[0] = camera()->position().x();
	config_data_.camera_pos.s[1] = camera()->position().y();
	config_data_.camera_pos.s[2] = camera()->position().z();
	config_data_.camera_dir.s[0] = camera()->direction().x();
	config_data_.camera_dir.s[1] = camera()->direction().y();
	config_data_.camera_dir.s[2] = camera()->direction().z();
	config_data_.camera_right.s[0] = camera()->right().x();
	config_data_.camera_right.s[1] = camera()->right().y();
	config_data_.camera_right.s[2] = camera()->right().z();
	config_data_.camera_up.s[0] = camera()->up().x();
	config_data_.camera_up.s[1] = camera()->up().y();
	config_data_.camera_up.s[2] = camera()->up().z();
	config_data_.camera_near_z = camera()->near_z();
	config_data_.camera_pixel_size = camera()->pixel_size();

	CHECK_ERROR(clEnqueueWriteBuffer(command_queue_, config_, CL_FALSE, 0, sizeof(config_data_), &config_data_, 0, nullptr, nullptr), "Cannot update param buffer");
}

opencl_render::~opencl_render()
{
	/// TODO: implement RAII for CL objects
	glDeleteTextures(1, &gl_tex_);

#ifdef INDIRECT_PARAMS
	glDeleteBuffers(1, &gl_atomic_counter_);
#endif

	glDeleteBuffers(1, &gl_buffer_);
	//clReleaseMemObject(bvh_texture_);
	clReleaseMemObject(visibility_buffer_);
	clReleaseMemObject(offsets_);
	clReleaseMemObject(atomic_counter_);
	clReleaseMemObject(bounds_);
	clReleaseMemObject(cull_result_);
	clReleaseMemObject(output_);
	clReleaseMemObject(vertices_);
	clReleaseMemObject(indices_);
	clReleaseMemObject(bvh_);
	clReleaseMemObject(config_);
	clReleaseContext(context_);
	clReleaseProgram(rt_program_);
	clReleaseCommandQueue(command_queue_);
	clReleaseKernel(rt_kernel_);
	clReleaseKernel(visibility_check_kernel_);
	clReleaseKernel(command_list_kernel_);
}

void opencl_render::render_and_cull(matrix4x4 const& mvp, std::vector<scene_base::mesh_desc> const& meshes)
{
	cl_event kernel_execution_event;

	glFinish();

	cl_mem gl_objects[] = {output_, cull_result_, atomic_counter_};

	CHECK_ERROR(clEnqueueAcquireGLObjects(command_queue_, 2, gl_objects, 0,0,0), "Cannot acquire OpenGL objects");

	size_t local_work_size[2];

	if (device_type_ == CL_DEVICE_TYPE_CPU)
	{
		local_work_size[0] = local_work_size[1] = 1;
	}
	else
	{
		local_work_size[0] = local_work_size[1] = 8;
	}

	size_t global_work_size[2] = {
		(output_size_.s[0] + local_work_size[0] - 1)/(local_work_size[0]) * local_work_size[0] , 
		(output_size_.s[1] + local_work_size[1] - 1)/(local_work_size[1]) * local_work_size[1]
	};

	CHECK_ERROR(clSetKernelArg(rt_kernel_, 0, sizeof(cl_mem), &bvh_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(rt_kernel_, 1, sizeof(cl_mem), &vertices_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(rt_kernel_, 2, sizeof(cl_mem), &indices_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(rt_kernel_, 3, sizeof(cl_mem), &config_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(rt_kernel_, 4, sizeof(cl_mem), &output_), "SetKernelArg failed");
	//CHECK_ERROR(clSetKernelArg(rt_kernel_, 5, sizeof(cl_int) * local_work_size[0] * local_work_size[1] * 64, nullptr), "SetKernelArg failed");

	cl_int status = clEnqueueNDRangeKernel(command_queue_, rt_kernel_, 2, nullptr, global_work_size, local_work_size, 0, nullptr, &kernel_execution_event);
	CHECK_ERROR(status, "Raytracing kernel launch failed");

	cull(mvp, meshes);

	CHECK_ERROR(clEnqueueReleaseGLObjects(command_queue_, 2, gl_objects, 0,0,0), "Cannot release OpenGL objects");
	CHECK_ERROR(clFinish(command_queue_), "Cannot flush command queue");

	CHECK_ERROR(clWaitForEvents(1, &kernel_execution_event), "Wait for events failed");

	cl_ulong time_start, time_end;
	double total_time;

	clGetEventProfilingInfo(kernel_execution_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, nullptr);
	clGetEventProfilingInfo(kernel_execution_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, nullptr);
	total_time = (double)(time_end - time_start)/1000000.0;

	std::cout << "Ray tracing time " << total_time << " msec\n";
}

void opencl_render::cull(matrix4x4 const& mvp, std::vector<scene_base::mesh_desc> const& meshes)
{
	cl_event kernel_execution_event[2];

	cl_uint num_meshes = meshes.size();

	assert(num_meshes < MAX_BOUNDS);

	std::vector<cl_float4> temp;
	std::vector<offset> temp1;
	std::for_each(meshes.begin(), meshes.end(), [&](scene_base::mesh_desc const& md)
	{
		cl_float4 sphere;
		sphere.s[0] = md.bsphere.center.x();
		sphere.s[1] = md.bsphere.center.y();
		sphere.s[2] = md.bsphere.center.z();
		sphere.s[3] = md.bsphere.radius;

		temp.push_back(sphere);

		offset o;
		o.start_idx = md.start_idx;
		o.num_idx   = md.num_idx;
		temp1.push_back(o);
	});

	CHECK_ERROR(clEnqueueWriteBuffer(command_queue_, bounds_, CL_FALSE, 0, sizeof(cl_float4) * temp.size(), &temp[0], 0, nullptr, nullptr), "Cannot update bounds buffer");
	CHECK_ERROR(clEnqueueWriteBuffer(command_queue_, offsets_, CL_FALSE, 0, sizeof(offset) * temp1.size(), &temp1[0], 0, nullptr, nullptr), "Cannot update offsets buffer");
	
	cl_int zero = 0;
	CHECK_ERROR(clEnqueueFillBuffer(command_queue_, visibility_buffer_, &zero, sizeof(cl_int), 0, sizeof(cl_int) * MAX_BOUNDS, 0, nullptr, nullptr), "Cannot clear visibility buffer");

	{
		size_t local_work_size[2] = {TILE_SIZE, TILE_SIZE};
		size_t global_work_size[2] = {
			(config_data_.output_width + local_work_size[0] - 1)/(local_work_size[0]) * local_work_size[0],
			(config_data_.output_height + local_work_size[1] - 1)/(local_work_size[1]) * local_work_size[1]
		};

		const cl_float16* mat = (const cl_float16*)&mvp;
		CHECK_ERROR(clSetKernelArg(visibility_check_kernel_, 0, sizeof(cl_float16), mat), "SetKernelArg failed")
		CHECK_ERROR(clSetKernelArg(visibility_check_kernel_, 1, sizeof(cl_uint), &num_meshes), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(visibility_check_kernel_, 2, sizeof(cl_mem), &bounds_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(visibility_check_kernel_, 3, sizeof(cl_mem), &output_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(visibility_check_kernel_, 4, sizeof(cl_mem), &visibility_buffer_), "SetKernelArg failed");

		cl_int status = clEnqueueNDRangeKernel(command_queue_, visibility_check_kernel_, 2, nullptr, global_work_size, local_work_size, 0, nullptr, &kernel_execution_event[0]);
		CHECK_ERROR(status, "Visibility check kernel launch failed");
	}

	CHECK_ERROR(clEnqueueFillBuffer(command_queue_, atomic_counter_, &zero, sizeof(cl_int), 0, sizeof(cl_int) , 0, nullptr, nullptr), "Cannot clear atomic counter");

	{
		size_t local_work_size[1] = {8};
		size_t global_work_size[1] = {
		(num_meshes + local_work_size[0] - 1)/(local_work_size[0]) * local_work_size[0]};

		CHECK_ERROR(clSetKernelArg(command_list_kernel_, 0, sizeof(cl_uint), &num_meshes), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(command_list_kernel_, 1, sizeof(cl_mem), &offsets_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(command_list_kernel_, 2, sizeof(cl_mem), &cull_result_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(command_list_kernel_, 3, sizeof(cl_mem), &atomic_counter_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(command_list_kernel_, 4, sizeof(cl_mem), &visibility_buffer_), "SetKernelArg failed");

		cl_int status = clEnqueueNDRangeKernel(command_queue_, command_list_kernel_, 1, nullptr, global_work_size, local_work_size, 0, nullptr, &kernel_execution_event[1]);
		CHECK_ERROR(status, "Command list kernel launch failed");
	}

	CHECK_ERROR(clEnqueueReadBuffer(command_queue_, atomic_counter_, CL_TRUE, 0, sizeof(int), &num_objects_, 0, nullptr, nullptr), "Cannot read back atomic counter");

	CHECK_ERROR(clWaitForEvents(2, kernel_execution_event), "Wait for events failed");

	cl_ulong time_start, time_end;
	double total_time;

	clGetEventProfilingInfo(kernel_execution_event[0], CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, nullptr);
	clGetEventProfilingInfo(kernel_execution_event[0], CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, nullptr);
	total_time = (double)(time_end - time_start)/1000000.0;

	std::cout << "Visibility check " << total_time << " msec\n";

	clGetEventProfilingInfo(kernel_execution_event[1], CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, nullptr);
	clGetEventProfilingInfo(kernel_execution_event[1], CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, nullptr);
	total_time = (double)(time_end - time_start)/1000000.0;

	std::cout << "Command list building " << total_time << " msec\n";
}

GLuint opencl_render::draw_command_buffer() const
{
	return gl_buffer_;
}

GLuint opencl_render::draw_command_count() const
{
#ifdef INDIRECT_PARAMS
	return gl_atomic_counter_;
#else
	return num_objects_;
#endif
}

