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

#include "render_base.h"
#include "camera_base.h"

#include "utils.h"
#include "bbox.h"

#define CHECK_ERROR(x) if((x) != CL_SUCCESS) throw std::runtime_error("OpenCL error occured");


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
#ifndef WIN32
    output_size_.x = width;
    output_size_.y = height;
#else
	output_size_.s[0] = width;
    output_size_.s[1] = height;
#endif

    cl_int status = CL_SUCCESS;
    CHECK_ERROR(clGetDeviceIDs(platform_, CL_DEVICE_TYPE_ALL, 1, &device_, nullptr));

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
    
    CHECK_ERROR(status);
    
    command_queue_ = clCreateCommandQueue(context_, device_, CL_QUEUE_PROFILING_ENABLE, &status);
    
    CHECK_ERROR(status);
    
    std::vector<char> source_code;
    load_file_contents("depthmap.cl", source_code);
    
    char* program_source = &source_code[0];
    size_t program_size = source_code.size();
    
    rt_program_ = clCreateProgramWithSource(context_, 1, (const char**)&program_source, &program_size, &status);
    CHECK_ERROR(status);
    
    if (clBuildProgram(rt_program_, 1, &device_, nullptr, nullptr, nullptr) < 0)
    {
        std::vector<char> build_log;
        size_t log_size;
        clGetProgramBuildInfo(rt_program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        
        build_log.resize(log_size);
        clGetProgramBuildInfo(rt_program_, device_, CL_PROGRAM_BUILD_LOG, log_size, &build_log[0], nullptr);
        
        throw std::runtime_error(&build_log[0]);
    };
    
    rt_kernel_ = clCreateKernel(rt_program_, "k", &status);
    CHECK_ERROR(status);
    
    accel_ = bvh_accel::create_from_scene(*scene());

    std::vector<cl_float4> vertices;
    std::for_each(accel_->vertices().cbegin(), accel_->vertices().cend(), [&vertices](vector3 const& v)
                  {
                      cl_float4 val;
#ifndef WIN32
                      val.x = v.x();
                      val.y = v.y();
                      val.z = v.z();
#else
                      val.s[0] = v.x();
                      val.s[1] = v.y();
                      val.s[2] = v.z();
#endif
                      vertices.push_back(val);
                  });
    
    
    vertices_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * vertices.size(), (void*)&vertices[0], &status);
    CHECK_ERROR(status);
    
    std::vector<cl_uint4> triangles;
    
    std::for_each(accel_->primitives().cbegin(), accel_->primitives().cend(), [&triangles](bvh_accel::triangle const& t)
                  {
                      cl_uint4 val;
#ifndef WIN32
                      val.x = t.i1;
                      val.y = t.i2;
                      val.z = t.i3;
#else
                      val.s[0] = t.i1;
                      val.s[1] = t.i2;
                      val.s[2] = t.i3;
#endif
                      triangles.push_back(val);
                  });
    
    indices_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint4) * triangles.size(), (void*)&triangles[0], &status);
    CHECK_ERROR(status);
    
    std::vector<bvh_node> nodes(accel_->nodes().size());
    std::transform(accel_->nodes().begin(), accel_->nodes().end(), nodes.begin(),
                   [](bvh_accel::node const& n)->bvh_node
                   {
                       bvh_node nn;
#ifndef WIN32
                       nn.box.pmin.x = n.box.min().x();
                       nn.box.pmin.y = n.box.min().y();
                       nn.box.pmin.z = n.box.min().z();
                       nn.box.pmax.x = n.box.max().x();
                       nn.box.pmax.y = n.box.max().y();
                       nn.box.pmax.z = n.box.max().z();
#else
					   nn.box.pmin.s[0] = n.box.min().x();
                       nn.box.pmin.s[1] = n.box.min().y();
                       nn.box.pmin.s[2] = n.box.min().z();
                       nn.box.pmax.s[0] = n.box.max().x();
                       nn.box.pmax.s[1] = n.box.max().y();
                       nn.box.pmax.s[2] = n.box.max().z();
#endif
                       nn.right = n.right;
                       nn.prim_start = n.prim_start_index;
                       nn.parent = n.parent;
                       nn.num_prims = n.num_prims;
                       
                       return nn;
                   }
                   );
    
    bvh_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR , nodes.size() * sizeof(bvh_node), (void*)&nodes[0], &status);
    CHECK_ERROR(status);
    
    config_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(config_data_), nullptr, &status);
    CHECK_ERROR(status);
    
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &gl_tex_);
    
    glBindTexture(GL_TEXTURE_2D, gl_tex_);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
#ifndef WIN32
		output_size_.x, output_size_.y,
#else
		output_size_.s[0], output_size_.s[1],
#endif
                 0, GL_RGBA, GL_FLOAT, nullptr);


    output_ = clCreateFromGLTexture(context_, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, gl_tex_, &status);
    CHECK_ERROR(status);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void opencl_render::commit()
{
#ifndef WIN32
    config_data_.output_width = output_size_.x;
    config_data_.output_height = output_size_.y;
    config_data_.camera_pos.x = camera()->position().x();
    config_data_.camera_pos.y = camera()->position().y();
    config_data_.camera_pos.z = camera()->position().z();
    config_data_.camera_dir.x = camera()->direction().x();
    config_data_.camera_dir.y = camera()->direction().y();
    config_data_.camera_dir.z = camera()->direction().z();
    config_data_.camera_right.x = camera()->right().x();
    config_data_.camera_right.y = camera()->right().y();
    config_data_.camera_right.z = camera()->right().z();
    config_data_.camera_up.x = camera()->up().x();
    config_data_.camera_up.y = camera()->up().y();
    config_data_.camera_up.z = camera()->up().z();
    config_data_.camera_near_z = camera()->near_z();
#else
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
#endif
    config_data_.camera_near_z = camera()->near_z();
    config_data_.camera_pixel_size = camera()->pixel_size();
    
    CHECK_ERROR(clEnqueueWriteBuffer(command_queue_, config_, CL_FALSE, 0, sizeof(config_data_), &config_data_, 0, nullptr, nullptr));
}

opencl_render::~opencl_render()
{
    /// TODO: implement RAII for CL objects
    clReleaseMemObject(output_);
    clReleaseMemObject(vertices_);
    clReleaseMemObject(indices_);
    clReleaseMemObject(bvh_);
    clReleaseMemObject(config_);
    clReleaseContext(context_);
    clReleaseProgram(rt_program_);
    clReleaseCommandQueue(command_queue_);
    clReleaseKernel(rt_kernel_);
}

void opencl_render::render()
{
    cl_event kernel_execution_event;
    
    glFinish();
    
    CHECK_ERROR(clEnqueueAcquireGLObjects(command_queue_, 1, &output_, 0,0,0));
    
    CHECK_ERROR(clSetKernelArg(rt_kernel_, 0, sizeof(cl_mem), &bvh_));
    CHECK_ERROR(clSetKernelArg(rt_kernel_, 1, sizeof(cl_mem), &vertices_));
    CHECK_ERROR(clSetKernelArg(rt_kernel_, 2, sizeof(cl_mem), &indices_));
    CHECK_ERROR(clSetKernelArg(rt_kernel_, 3, sizeof(cl_mem), &config_));
    CHECK_ERROR(clSetKernelArg(rt_kernel_, 4, sizeof(cl_mem), &output_));

    size_t local_work_size[2];
    
    if (device_type_ == CL_DEVICE_TYPE_CPU)
    {
        local_work_size[0] = local_work_size[1] = 1;
    }
    else
    {
        local_work_size[0] = local_work_size[1] = 16;
    }
    
    size_t global_work_size[2] = {
#ifndef WIN32
		(output_size_.x + local_work_size[0] - 1)/(local_work_size[0]) * local_work_size[0], 
		(output_size_.y + local_work_size[1] - 1)/(local_work_size[1]) * local_work_size[1]
#else
		(output_size_.s[0] + local_work_size[0] - 1)/(local_work_size[0]) * local_work_size[0] , 
		(output_size_.s[1] + local_work_size[1] - 1)/(local_work_size[1]) * local_work_size[1]
#endif
};
    
    cl_int status = clEnqueueNDRangeKernel(command_queue_, rt_kernel_, 2, nullptr, global_work_size, local_work_size, 0, nullptr, &kernel_execution_event);
    CHECK_ERROR(status);
    
    CHECK_ERROR(clEnqueueReleaseGLObjects(command_queue_, 1, &output_, 0,0,0));
    CHECK_ERROR(clFinish(command_queue_));
    
    CHECK_ERROR(clWaitForEvents(1, &kernel_execution_event));
    
    cl_ulong time_start, time_end;
    double total_time;
    
    clGetEventProfilingInfo(kernel_execution_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, nullptr);
    clGetEventProfilingInfo(kernel_execution_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, nullptr);
    total_time = (double)(time_end - time_start)/1000000.0;
    
    std::cout << "BVH traversal time " << total_time << " msec\n";
}


