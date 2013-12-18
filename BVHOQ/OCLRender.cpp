//
//  OCLRender.cpp
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "OCLRender.h"

#include <limits>
#include <chrono>
#include <iostream>
#include <string>
#include <sstream>

#include "RenderBase.h"
#include "CameraBase.h"

#include "utils.h"
#include "bbox.h"

#define CHECK_ERROR(x,m) if((x) != CL_SUCCESS) { std::ostringstream o; o << m <<" error " <<x <<"\n";  throw std::runtime_error(o.str()); }

#define MAX_BOUNDS 1000
#define TILE_SIZE 8

GLuint OCLRender::GetOutputTexture() const
{
	return glDepthTexture_;
}

OCLRender::OCLRender(cl_platform_id platform)
	:platform_(platform)
	, visibleObjectsCount_(0)
{
	
}

void OCLRender::Init(unsigned width, unsigned height)
{
	outputSize_.s[0] = width;
	outputSize_.s[1] = height;

	cl_int status = CL_SUCCESS;
	CHECK_ERROR(clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, 1, &device_, nullptr), "GetDeviceIDs failed");

	char device_name[2048];
	clGetDeviceInfo(device_, CL_DEVICE_NAME, 2048, device_name, nullptr);
	std::cout << "Device detected: " << device_name << "\n";

	size_t workGroupSize = 0;
	clGetDeviceInfo(device_, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, nullptr);
	std::cout << "Max work group size: " << workGroupSize << "\n";
	
	clGetDeviceInfo(device_, CL_DEVICE_TYPE, sizeof(deviceType_), &deviceType_, nullptr);

	switch (deviceType_)
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
	
	commandQueue_ = clCreateCommandQueue(context_, device_, CL_QUEUE_PROFILING_ENABLE, &status);
	
	CHECK_ERROR(status, "Cannot create command queue");
	
	std::vector<char> sourceCode;
	LoadFileContents("depthmap.cl", sourceCode);
	
	char*  programSource = &sourceCode[0];
	size_t programSize = sourceCode.size();
	
	program_ = clCreateProgramWithSource(context_, 1, (const char**)&programSource, &programSize, &status);
	CHECK_ERROR(status, "Cannnot create program from depthmap.cl");
	
	if (clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr) < 0)
	{
		std::vector<char> buildLog;
		size_t logSize;
		clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
		
		buildLog.resize(logSize);
		clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, logSize, &buildLog[0], nullptr);
		
		throw std::runtime_error(&buildLog[0]);
	};
	
	traceDepthKernel_ = clCreateKernel(program_, "TraceDepth", &status);
	CHECK_ERROR(status, "Cannot create TraceDepth kernel");
	
	checkVisibilityKernel_ = clCreateKernel(program_, "CheckVisibility", &status);
	CHECK_ERROR(status, "Cannot create CheckVisibility kernel");

	buildCmdListKernel_ = clCreateKernel(program_, "BuildCmdList", &status);
	CHECK_ERROR(status, "Cannot create BuildCmdList kernel");
	
	accel_ = BVHAccelerator::CreateFromScene(*GetScene());

	std::vector<cl_float4> vertices;
	std::for_each(accel_->GetVertices().cbegin(), accel_->GetVertices().cend(), [&vertices](vector3 const& v)
				  {
					  cl_float4 val;

					  val.s[0] = v.x();
					  val.s[1] = v.y();
					  val.s[2] = v.z();

					  vertices.push_back(val);
				  });
	
	
	vertexBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * vertices.size(), (void*)&vertices[0], &status);
	CHECK_ERROR(status, "Cannot create vertex buffer");
	
	std::vector<cl_uint4> triangles;
	
	std::for_each(accel_->GetPrimitives().cbegin(), accel_->GetPrimitives().cend(), [&triangles](BVHAccelerator::Triangle const& t)
				  {
					  cl_uint4 val;

					  val.s[0] = t.i1;
					  val.s[1] = t.i2;
					  val.s[2] = t.i3;

					  triangles.push_back(val);
				  });
	
	indexBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint4) * triangles.size(), (void*)&triangles[0], &status);
	CHECK_ERROR(status, "Cannot create index buffer");
	
	std::vector<DevBVHNode> nodes(accel_->GetNodes().size());
	std::transform(accel_->GetNodes().begin(), accel_->GetNodes().end(), nodes.begin(),
		[](BVHAccelerator::Node const& n)->DevBVHNode
				   {
					   DevBVHNode nn;

					   nn.sBox.vMin.s[0] = n.box.GetMinPoint().x();
					   nn.sBox.vMin.s[1] = n.box.GetMinPoint().y();
					   nn.sBox.vMin.s[2] = n.box.GetMinPoint().z();
					   nn.sBox.vMax.s[0] = n.box.GetMaxPoint().x();
					   nn.sBox.vMax.s[1] = n.box.GetMaxPoint().y();
					   nn.sBox.vMax.s[2] = n.box.GetMaxPoint().z();

					   nn.uRight = n.right;
					   nn.uPrimStart = n.primStartIdx;
					   nn.uParent = n.parent;
					   nn.uPrimCount = n.primCount;
					   
					   return nn;
				   }
				   );

	bvhBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR , nodes.size() * sizeof(DevBVHNode), (void*)&nodes[0], &status);
	CHECK_ERROR(status, "Cannot create BVH node buffer");
	
	configBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(configData_), nullptr, &status);
	CHECK_ERROR(status, "Cannot create parameter buffer");

	boundsBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(cl_float4) * MAX_BOUNDS, nullptr, &status);
	CHECK_ERROR(status, "Cannot create bounds buffer");

	offsetsBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(DevOffsets) * MAX_BOUNDS, nullptr, &status);
	CHECK_ERROR(status, "Cannot create offsets buffer");

	visibilityBuffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_uint) * MAX_BOUNDS, nullptr, &status);
	CHECK_ERROR(status, "Cannot create visibility buffer");

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &glDepthTexture_);

	glBindTexture(GL_TEXTURE_2D, glDepthTexture_);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, outputSize_.s[0], outputSize_.s[1], 0, GL_RGBA, GL_FLOAT, nullptr);

	outputDepthTexture_ = clCreateFromGLTexture(context_, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, glDepthTexture_, &status);
	CHECK_ERROR(status, "Cannot create interop texture");
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenBuffers(1, &glCmdBuffer_);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, glCmdBuffer_);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DevDrawCommand) * MAX_BOUNDS, nullptr, GL_STATIC_READ);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	drawCmdBuffer_ = clCreateFromGLBuffer(context_, CL_MEM_WRITE_ONLY, glCmdBuffer_, &status);
	CHECK_ERROR(status, "Cannot create interop buffer");

#ifdef INDIRECT_PARAMS
	glGenBuffers(1, &glVisibleObjectsCounterBuffer_);

	glBindBuffer(GL_PARAMETER_BUFFER_ARB, glVisibleObjectsCounterBuffer_);
	glBufferData(GL_PARAMETER_BUFFER_ARB, sizeof(cl_uint), nullptr, GL_STATIC_READ);
	glBindBuffer(GL_PARAMETER_BUFFER_ARB, 0);


	visibleObjectsCounterBuffer_ = clCreateFromGLBuffer(context_, CL_MEM_WRITE_ONLY, glVisibleObjectsCounterBuffer_, &status);
	CHECK_ERROR(status, "Cannot create atomic counter interop buffer");
#else
	visibleObjectsCounterBuffer_ =  clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &status);
	CHECK_ERROR(status, "Cannot create atomic counter buffer");
#endif
}

void OCLRender::Commit()
{
	matrix4x4 mView = GetCamera()->GetViewMatrix();
	matrix4x4 mProj = GetCamera()->GetProjMatrix();
	matrix4x4 mProjInv = inverse(mProj);

	memcpy(&configData_.mView, &mView(0,0), 4*4*sizeof(float));
	memcpy(&configData_.mProjInv, &mProjInv(0,0), 4*4*sizeof(float));

	configData_.uOutputWidth = outputSize_.s[0];
	configData_.uOutputHeight = outputSize_.s[1];
	configData_.vCameraPos.s[0] = GetCamera()->GetPosition().x();
	configData_.vCameraPos.s[1] = GetCamera()->GetPosition().y();
	configData_.vCameraPos.s[2] = GetCamera()->GetPosition().z();
	configData_.vCameraDir.s[0] = GetCamera()->GetDirection().x();
	configData_.vCameraDir.s[1] = GetCamera()->GetDirection().y();
	configData_.vCameraDir.s[2] = GetCamera()->GetDirection().z();
	configData_.vCameraRight.s[0] = GetCamera()->GetRightVector().x();
	configData_.vCameraRight.s[1] = GetCamera()->GetRightVector().y();
	configData_.vCameraRight.s[2] = GetCamera()->GetRightVector().z();
	configData_.vCameraUp.s[0] = GetCamera()->GetUpVector().x();
	configData_.vCameraUp.s[1] = GetCamera()->GetUpVector().y();
	configData_.vCameraUp.s[2] = GetCamera()->GetUpVector().z();
	configData_.fCameraNearZ = GetCamera()->GetNearZ();
	configData_.fCameraPixelSize = GetCamera()->GetPixelSize();

	CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, configBuffer_, CL_FALSE, 0, sizeof(configData_), &configData_, 0, nullptr, nullptr), "Cannot update param buffer");
}

OCLRender::~OCLRender()
{
	/// TODO: implement RAII for CL objects
	glDeleteTextures(1, &glDepthTexture_);

#ifdef INDIRECT_PARAMS
	glDeleteBuffers(1, &glVisibleObjectsCounterBuffer_);
#endif

	glDeleteBuffers(1, &glCmdBuffer_);
	//clReleaseMemObject(bvh_texture_);
	clReleaseMemObject(visibilityBuffer_);
	clReleaseMemObject(offsetsBuffer_);
	clReleaseMemObject(visibleObjectsCounterBuffer_);
	clReleaseMemObject(boundsBuffer_);
	clReleaseMemObject(drawCmdBuffer_);
	clReleaseMemObject(outputDepthTexture_);
	clReleaseMemObject(vertexBuffer_);
	clReleaseMemObject(indexBuffer_);
	clReleaseMemObject(bvhBuffer_);
	clReleaseMemObject(configBuffer_);
	clReleaseContext(context_);
	clReleaseProgram(program_);
	clReleaseCommandQueue(commandQueue_);
	clReleaseKernel(traceDepthKernel_);
	clReleaseKernel(checkVisibilityKernel_);
	clReleaseKernel(buildCmdListKernel_);
}

void OCLRender::CullMeshes(std::vector<SceneBase::MeshDesc> const& meshes)
{
	cl_event kernelExecutionEvent;

	glFinish();

	cl_mem gl_objects[] = {outputDepthTexture_, drawCmdBuffer_, visibleObjectsCounterBuffer_};

	CHECK_ERROR(clEnqueueAcquireGLObjects(commandQueue_, 2, gl_objects, 0,0,0), "Cannot acquire OpenGL objects");

	size_t localWorkSize[2];

	if (deviceType_ == CL_DEVICE_TYPE_CPU)
	{
		localWorkSize[0] = localWorkSize[1] = 1;
	}
	else
	{
		localWorkSize[0] = localWorkSize[1] = 8;
	}

	size_t globalWorkSize[2] = {
		(outputSize_.s[0] + localWorkSize[0] - 1)/(localWorkSize[0]) * localWorkSize[0] , 
		(outputSize_.s[1] + localWorkSize[1] - 1)/(localWorkSize[1]) * localWorkSize[1]
	};

	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 0, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 1, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 2, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 3, sizeof(cl_mem), &configBuffer_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 4, sizeof(cl_mem), &outputDepthTexture_), "SetKernelArg failed");
	//CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 5, sizeof(cl_int) * localWorkSize[0] * localWorkSize[1] * 64, nullptr), "SetKernelArg failed");

	cl_int status = clEnqueueNDRangeKernel(commandQueue_, traceDepthKernel_, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, &kernelExecutionEvent);
	CHECK_ERROR(status, "Raytracing kernel launch failed");

	Cull(meshes);

	CHECK_ERROR(clEnqueueReleaseGLObjects(commandQueue_, 2, gl_objects, 0,0,0), "Cannot release OpenGL objects");
	CHECK_ERROR(clFinish(commandQueue_), "Cannot flush command queue");

	CHECK_ERROR(clWaitForEvents(1, &kernelExecutionEvent), "Wait for events failed");

	cl_ulong startTime, endTime;
	double totalTime;

	clGetEventProfilingInfo(kernelExecutionEvent, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
	clGetEventProfilingInfo(kernelExecutionEvent, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
	totalTime = (double)(endTime - startTime)/1000000.0;

	std::cout << "Ray tracing time " << totalTime << " msec\n";
}

void OCLRender::Cull(std::vector<SceneBase::MeshDesc> const& meshes)
{
	cl_event kernelExecutionEvent[2];

	cl_uint numMeshes = meshes.size();

	assert(numMeshes < MAX_BOUNDS);

	std::vector<cl_float4> temp;
	std::vector<DevOffsets> temp1;
	std::for_each(meshes.begin(), meshes.end(), [&](SceneBase::MeshDesc const& md)
	{
		cl_float4 sphere;
		sphere.s[0] = md.bSphere.center.x();
		sphere.s[1] = md.bSphere.center.y();
		sphere.s[2] = md.bSphere.center.z();
		sphere.s[3] = md.bSphere.radius;

		temp.push_back(sphere);

		DevOffsets o;
		o.uStartIdx = md.startIdx;
		o.uNumIndices   = md.numIndices;
		temp1.push_back(o);
	});

	CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, boundsBuffer_, CL_FALSE, 0, sizeof(cl_float4) * temp.size(), &temp[0], 0, nullptr, nullptr), "Cannot update bounds buffer");
	CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, offsetsBuffer_, CL_FALSE, 0, sizeof(DevOffsets) * temp1.size(), &temp1[0], 0, nullptr, nullptr), "Cannot update offsets buffer");
	
	cl_int zero = 0;
	CHECK_ERROR(clEnqueueFillBuffer(commandQueue_, visibilityBuffer_, &zero, sizeof(cl_int), 0, sizeof(cl_int) * MAX_BOUNDS, 0, nullptr, nullptr), "Cannot clear visibility buffer");

	{
		size_t localWorkSize[2] = {TILE_SIZE, TILE_SIZE};
		size_t globalWorkSize[2] = {
			(configData_.uOutputWidth + localWorkSize[0] - 1)/(localWorkSize[0]) * localWorkSize[0],
			(configData_.uOutputHeight + localWorkSize[1] - 1)/(localWorkSize[1]) * localWorkSize[1]
		};

		//const cl_float16* mat = (const cl_float16*)&mvp;
		CHECK_ERROR(clSetKernelArg(checkVisibilityKernel_, 0, sizeof(cl_mem), &configBuffer_), "SetKernelArg failed")
		CHECK_ERROR(clSetKernelArg(checkVisibilityKernel_, 1, sizeof(cl_uint), &numMeshes), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(checkVisibilityKernel_, 2, sizeof(cl_mem), &boundsBuffer_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(checkVisibilityKernel_, 3, sizeof(cl_mem), &outputDepthTexture_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(checkVisibilityKernel_, 4, sizeof(cl_mem), &visibilityBuffer_), "SetKernelArg failed");

		cl_int status = clEnqueueNDRangeKernel(commandQueue_, checkVisibilityKernel_, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, &kernelExecutionEvent[0]);
		CHECK_ERROR(status, "Visibility check kernel launch failed");
	}

	CHECK_ERROR(clEnqueueFillBuffer(commandQueue_, visibleObjectsCounterBuffer_, &zero, sizeof(cl_int), 0, sizeof(cl_int) , 0, nullptr, nullptr), "Cannot clear atomic counter");

	{
		size_t localWorkSize[1] = {8};
		size_t globalWorkSize[1] = {
		(numMeshes + localWorkSize[0] - 1)/(localWorkSize[0]) * localWorkSize[0]};

		CHECK_ERROR(clSetKernelArg(buildCmdListKernel_, 0, sizeof(cl_uint), &numMeshes), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(buildCmdListKernel_, 1, sizeof(cl_mem), &offsetsBuffer_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(buildCmdListKernel_, 2, sizeof(cl_mem), &drawCmdBuffer_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(buildCmdListKernel_, 3, sizeof(cl_mem), &visibleObjectsCounterBuffer_), "SetKernelArg failed");
		CHECK_ERROR(clSetKernelArg(buildCmdListKernel_, 4, sizeof(cl_mem), &visibilityBuffer_), "SetKernelArg failed");

		cl_int status = clEnqueueNDRangeKernel(commandQueue_, buildCmdListKernel_, 1, nullptr, globalWorkSize, localWorkSize, 0, nullptr, &kernelExecutionEvent[1]);
		CHECK_ERROR(status, "Command list kernel launch failed");
	}

	CHECK_ERROR(clEnqueueReadBuffer(commandQueue_, visibleObjectsCounterBuffer_, CL_TRUE, 0, sizeof(int), &visibleObjectsCount_, 0, nullptr, nullptr), "Cannot read back atomic counter");

	CHECK_ERROR(clWaitForEvents(2, kernelExecutionEvent), "Wait for events failed");

	cl_ulong startTime, endTime;
	double totalTime;

	clGetEventProfilingInfo(kernelExecutionEvent[0], CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
	clGetEventProfilingInfo(kernelExecutionEvent[0], CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
	totalTime = (double)(endTime - startTime)/1000000.0;

	std::cout << "Visibility check " << totalTime << " msec\n";

	clGetEventProfilingInfo(kernelExecutionEvent[1], CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
	clGetEventProfilingInfo(kernelExecutionEvent[1], CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
	totalTime = (double)(endTime - startTime)/1000000.0;

	std::cout << "Command list building " << totalTime << " msec\n";
}

GLuint OCLRender::GetDrawCommandBuffer() const
{
	return glCmdBuffer_;
}

GLuint OCLRender::GetDrawCommandCount() const
{
#ifdef INDIRECT_PARAMS
	return glVisibleObjectsCounterBuffer_;
#else
	return visibleObjectsCount_;
#endif
}

