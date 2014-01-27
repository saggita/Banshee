//
//  OCLRender.cpp
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "OCLRender.h"

#include <limits>
#include <iostream>
#include <string>
#include <sstream>

#include "RenderBase.h"
#include "CameraBase.h"

#include "utils.h"
#include "bbox.h"

#define CHECK_ERROR(x,m) if((x) != CL_SUCCESS) { std::ostringstream o; o << m <<" error " <<x <<"\n";  throw std::runtime_error(o.str()); }

#define MAX_BOUNDS 1000
#define RANDOM_BUFFER_SIZE 1000
#define MAX_PATH_LENGTH 3

GLuint OCLRender::GetOutputTexture() const
{
	return glDepthTexture_;
}

OCLRender::OCLRender(cl_platform_id platform)
	:platform_(platform)
    , frameCount_(0)
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

	cl_ulong maxAllocationSize = 0;
	status = clGetDeviceInfo(device_, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &maxAllocationSize, nullptr);
	std::cout << "Max mem allocation size: " << maxAllocationSize << "\n";

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
	LoadFileContents("../../../FireRays/CL/raytrace.cl", sourceCode);
	
	char*  programSource = &sourceCode[0];
	size_t programSize = sourceCode.size();
	
	program_ = clCreateProgramWithSource(context_, 1, (const char**)&programSource, &programSize, &status);
	CHECK_ERROR(status, "Cannnot create program from raytrace.cl");
	
    const char* buildOptions
#ifdef __APPLE__
     = "-D OCLAPPLE";
#elif WIN32
    = "-D OCLWIN32";
#else
    = "";
#endif
    
	if (clBuildProgram(program_, 1, &device_, buildOptions, nullptr, nullptr) < 0)
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
		
	accel_ = BVHAccelerator::CreateFromScene(*GetScene());

	std::vector<DevVertex> vertices;
	std::for_each(GetScene()->GetVertices().cbegin(), GetScene()->GetVertices().cend(), [&vertices](SceneBase::Vertex const& v)
				  {
					  DevVertex val;

					  val.vPos.s[0] = v.position.x();
					  val.vPos.s[1] = v.position.y();
					  val.vPos.s[2] = v.position.z();
                      val.vPos.s[3] = 0;
                      
                      val.vNormal.s[0] = v.normal.x();
					  val.vNormal.s[1] = v.normal.y();
					  val.vNormal.s[2] = v.normal.z();
                      val.vNormal.s[3] = 0;

					  vertices.push_back(val);
				  });
	
	
	vertexBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(DevVertex) * vertices.size(), (void*)&vertices[0], &status);
	CHECK_ERROR(status, "Cannot create vertex buffer");
	
	std::vector<cl_uint4> triangles;
	
	std::for_each(accel_->GetPrimitives().cbegin(), accel_->GetPrimitives().cend(), [&triangles](BVHAccelerator::Triangle const& t)
				  {
					  cl_uint4 val;

					  val.s[0] = t.i1;
					  val.s[1] = t.i2;
					  val.s[2] = t.i3;
                      val.s[3] = t.m;

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

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &glDepthTexture_);

	glBindTexture(GL_TEXTURE_2D, glDepthTexture_);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, outputSize_.s[0], outputSize_.s[1], 0, GL_RGBA, GL_FLOAT, nullptr);

	outputDepthTexture_ = clCreateFromGLTexture(context_, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, glDepthTexture_, &status);
	CHECK_ERROR(status, "Cannot create interop texture");
	
	glBindTexture(GL_TEXTURE_2D, 0);
    
    std::vector<DevPointLight> pointLights;
    
    DevPointLight light;
    light.vPos.s[0] = -1.f;
    light.vPos.s[1] = 0.f;
    light.vPos.s[2] = -1.f;
    light.vPos.s[3] = 0.f;

    light.vColor.s[0] = 2.7f;
    light.vColor.s[1] = 2.7f;
    light.vColor.s[2] = 2.7f;
    light.vColor.s[3] = 0.f;
    
    pointLights.push_back(light);
    
    //DevPointLight light;
    light.vPos.s[0] = 1.f;
    light.vPos.s[1] = 2.f;
    light.vPos.s[2] = 1.f;
    light.vPos.s[3] = 0.f;
    
    light.vColor.s[0] = 1.5f;
    light.vColor.s[1] = 1.5f;
    light.vColor.s[2] = 1.5f;
    light.vColor.s[3] = 0.f;
    
    pointLights.push_back(light);
    
    pointLights_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(DevPointLight) * pointLights.size(), (void*)&pointLights[0],&status);
	CHECK_ERROR(status, "Cannot create lights buffer");
    
    std::vector<cl_float> randomBuffer(RANDOM_BUFFER_SIZE);
    std::generate(randomBuffer.begin(), randomBuffer.end(), rand_float);
    
    randomBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR , RANDOM_BUFFER_SIZE * sizeof(cl_float), (void*)&randomBuffer[0], &status);
	CHECK_ERROR(status, "Cannot create random buffer");

    pathBuffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, MAX_PATH_LENGTH * sizeof(DevPathVertex) * outputSize_.s[0] * outputSize_.s[1], nullptr, &status);
	CHECK_ERROR(status, "Cannot create path buffer");

	intermediateBuffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_float4) * outputSize_.s[0] * outputSize_.s[1], nullptr, &status);
	CHECK_ERROR(status, "Cannot create intermediate buffer");

	std::vector<DevMaterialRep> materials;

	DevMaterialRep materialRep;

	materialRep.eBsdf = 2;
	materialRep.vKd.s[0] = materialRep.vKd.s[1] = materialRep.vKd.s[2] = materialRep.vKd.s[3] = 0;
	materialRep.vKs.s[0] = 0.9; materialRep.vKs.s[1] = 0.8;
	materialRep.vKs.s[2] = materialRep.vKs.s[3] = 0;
	materials.push_back(materialRep);

	materialRep.eBsdf = 1;
	materialRep.vKd.s[0] = materialRep.vKd.s[1] = materialRep.vKd.s[2] = materialRep.vKd.s[3] = 0.6;
	materialRep.vKs.s[0] = materialRep.vKs.s[1] = materialRep.vKs.s[2] = materialRep.vKs.s[3] = 0.0;
	materials.push_back(materialRep);

	materialBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(DevMaterialRep) * 2, (void*)&materials[0], &status);
	CHECK_ERROR(status, "Cannot create material buffer");
}

void OCLRender::Commit()
{
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
    configData_.uNumPointLights = 2;
    configData_.uNumRandomNumbers = RANDOM_BUFFER_SIZE;
    configData_.uFrameCount = frameCount_;

	CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, configBuffer_, CL_FALSE, 0, sizeof(configData_), &configData_, 0, nullptr, nullptr), "Cannot update param buffer");
}

OCLRender::~OCLRender()
{
	/// TODO: implement RAII for CL objects
	glDeleteTextures(1, &glDepthTexture_);
	clReleaseMemObject(materialBuffer_);
	clReleaseMemObject(intermediateBuffer_);
    clReleaseMemObject(pathBuffer_);
    clReleaseMemObject(randomBuffer_);
    clReleaseMemObject(outputDepthTexture_);
    clReleaseMemObject(pointLights_);
	clReleaseMemObject(vertexBuffer_);
	clReleaseMemObject(indexBuffer_);
	clReleaseMemObject(bvhBuffer_);
	clReleaseMemObject(configBuffer_);
	clReleaseContext(context_);
	clReleaseProgram(program_);
	clReleaseCommandQueue(commandQueue_);
	clReleaseKernel(traceDepthKernel_);
}

void OCLRender::Render()
{
	cl_event kernelExecutionEvent;

	glFinish();

	cl_mem gl_objects[] = {outputDepthTexture_};

	CHECK_ERROR(clEnqueueAcquireGLObjects(commandQueue_, 1, gl_objects, 0,0,0), "Cannot acquire OpenGL objects");

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
    CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 4, sizeof(cl_mem), &pointLights_), "SetKernelArg failed");
    CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 5, sizeof(cl_mem), &randomBuffer_), "SetKernelArg failed");
    CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 6, sizeof(cl_mem), &pathBuffer_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 7, sizeof(cl_mem), &materialBuffer_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 8, sizeof(cl_mem), &intermediateBuffer_), "SetKernelArg failed");
	CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 9, sizeof(cl_mem), &outputDepthTexture_), "SetKernelArg failed");
	//CHECK_ERROR(clSetKernelArg(traceDepthKernel_, 5, sizeof(cl_int) * localWorkSize[0] * localWorkSize[1] * 64, nullptr), "SetKernelArg failed");

	cl_int status = clEnqueueNDRangeKernel(commandQueue_, traceDepthKernel_, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, &kernelExecutionEvent);
	CHECK_ERROR(status, "Raytracing kernel launch failed");

	CHECK_ERROR(clEnqueueReleaseGLObjects(commandQueue_, 1, gl_objects, 0,0,0), "Cannot release OpenGL objects");
	CHECK_ERROR(clFinish(commandQueue_), "Cannot flush command queue");

	CHECK_ERROR(clWaitForEvents(1, &kernelExecutionEvent), "Wait for events failed");

	cl_ulong startTime, endTime;
	double totalTime;

	clGetEventProfilingInfo(kernelExecutionEvent, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
	clGetEventProfilingInfo(kernelExecutionEvent, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
	totalTime = (double)(endTime - startTime)/1000000.0;

	std::cout << "Ray tracing time " << totalTime << " msec\n";
    
    ++frameCount_;
}

void   OCLRender::FlushFrame()
{
    frameCount_ = 0;
}



