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
#include <chrono>

#include "RenderBase.h"
#include "CameraBase.h"
#include "TextureBase.h"

#include "BVH.h"
#include "SplitBVHBuilder.h"
#include "LinearBVHBuilder.h"
#include "OCLBVHBackEnd.h"

#include "utils.h"
#include "bbox.h"

#define CHECK_ERROR(x,m) if((x) != CL_SUCCESS) { std::ostringstream o; o << m <<" error " <<x <<"\n";  throw std::runtime_error(o.str()); }

#define MAX_BOUNDS 1000
#define RANDOM_BUFFER_SIZE 1000
#define MAX_PATH_LENGTH 5
#define TEXTURE_BUFFER_SIZE (134217728 >> 2)
#define MAX_TEXTURE_HANDLES 100
#define MAX_AREA_LIGHTS 100

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
        = "";
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
    
    pathGenerationKernel_ = clCreateKernel(program_, "GeneratePath", &status);
    CHECK_ERROR(status, "Cannot create GeneratePath kernel");
    
    pathTraceKernel_ = clCreateKernel(program_, "TracePath", &status);
    CHECK_ERROR(status, "Cannot create TracePath kernel");

    pathShadeAndExportKernel_ = clCreateKernel(program_, "ShadeAndExport", &status);
    CHECK_ERROR(status, "Cannot create shade kernel");

    traceExperiments_ = clCreateKernel(program_, "TraceExperiments", &status);
    CHECK_ERROR(status, "Cannot create trace experiments kernel");
    
    BVH bvh;
    SplitBVHBuilder builder(GetScene()->GetVertices(), GetScene()->GetVertexCount(), GetScene()->GetIndices(), GetScene()->GetIndexCount(), GetScene()->GetMaterials(), 128U, 1.f, 1.f);
    //LinearBVHBuilder builder(GetScene()->GetVertices(), GetScene()->GetVertexCount(), GetScene()->GetIndices(), GetScene()->GetIndexCount(), GetScene()->GetMaterials());
    builder.SetBVH(&bvh);
    
    static auto prevTime = std::chrono::high_resolution_clock::now();

    builder.Build();
    
    auto currentTime = std::chrono::high_resolution_clock::now();
	auto deltaTime   = std::chrono::duration_cast<std::chrono::duration<double> >(currentTime - prevTime);
    
    std::cout << "\nBVH building time " << deltaTime.count() << " secs\n";
    
    OCLBVHBackEnd backEnd(bvh);
    backEnd.Generate();

    std::vector<DevVertex> vertices(GetScene()->GetVertexCount());
    SceneBase::Vertex const* sceneVertices = GetScene()->GetVertices(); 
    for (int i = 0; i < GetScene()->GetVertexCount(); ++i)
    {
        vertices[i].vPos.s[0] = sceneVertices[i].position.x();
        vertices[i].vPos.s[1] = sceneVertices[i].position.y();
        vertices[i].vPos.s[2] = sceneVertices[i].position.z();
        vertices[i].vPos.s[3] = 0;

        vertices[i].vNormal.s[0] = sceneVertices[i].normal.x();
        vertices[i].vNormal.s[1] = sceneVertices[i].normal.y();
        vertices[i].vNormal.s[2] = sceneVertices[i].normal.z();
        vertices[i].vNormal.s[3] = 0;

        vertices[i].vTex.s[0] = sceneVertices[i].texcoord.x();
        vertices[i].vTex.s[1] = sceneVertices[i].texcoord.y();
    };

    vertexBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(DevVertex) * vertices.size(), (void*)&vertices[0], &status);
    CHECK_ERROR(status, "Cannot create vertex buffer");

    std::vector<cl_uint4> triangles(builder.GetPrimitiveCount());
    std::vector<cl_uint>  areaLights;

    SplitBVHBuilder::Primitive const* sceneTriangles = builder.GetPrimitives();
    for (int i = 0; i < builder.GetPrimitiveCount(); ++i)
    {
        triangles[i].s[0] = sceneTriangles[i].i1;
        triangles[i].s[1] = sceneTriangles[i].i2;
        triangles[i].s[2] = sceneTriangles[i].i3;
        triangles[i].s[3] = sceneTriangles[i].m;

        if (sceneTriangles[i].m == 2)
            areaLights.push_back(i);
    }

    indexBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint4) * triangles.size(), (void*)&triangles[0], &status);
    CHECK_ERROR(status, "Cannot create index buffer");

    std::vector<DevBVHNode> nodes(backEnd.GetNodeCount());
    OCLBVHBackEnd::Node const* accelNodes = backEnd.GetNodes();
    for (int i = 0; i < backEnd.GetNodeCount(); ++i)
    {
        
        nodes[i].sBox.vMin.s[0] = accelNodes[i].box.min.x;
        nodes[i].sBox.vMin.s[1] = accelNodes[i].box.min.y;
        nodes[i].sBox.vMin.s[2] = accelNodes[i].box.min.z;
        nodes[i].sBox.vMax.s[0] = accelNodes[i].box.max.x;
        nodes[i].sBox.vMax.s[1] = accelNodes[i].box.max.y;
        nodes[i].sBox.vMax.s[2] = accelNodes[i].box.max.z;

        nodes[i].uRight     = accelNodes[i].right;
        nodes[i].uPrimStart = accelNodes[i].primStartIdx;
        nodes[i].uParent    = accelNodes[i].parent;
        nodes[i].uPrimCount = accelNodes[i].primCount;
    }

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
    light.vPos.s[0] = 0.f;
    light.vPos.s[1] = 9.f;
    light.vPos.s[2] = 0.f;
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

    std::vector<DevMaterialRep> materials(GetScene()->GetMaterialRepCount());

    SceneBase::MaterialRep const* sceneMaterials = GetScene()->GetMaterialReps();
    
    for (int i = 0; i < materials.size(); ++i)
    {
        materials[i].eBsdf = sceneMaterials[i].eBsdf;
        materials[i].vKe.s[0] = sceneMaterials[i].vKe.x();
        materials[i].vKe.s[1] = sceneMaterials[i].vKe.y();
        materials[i].vKe.s[2] = sceneMaterials[i].vKe.z();
        materials[i].vKe.s[3] = sceneMaterials[i].vKe.w();
        
        materials[i].vKd.s[0] = sceneMaterials[i].vKd.x();
        materials[i].vKd.s[1] = sceneMaterials[i].vKd.y();
        materials[i].vKd.s[2] = sceneMaterials[i].vKd.z();
        materials[i].vKd.s[3] = sceneMaterials[i].vKd.w();
        
        materials[i].vKs.s[0] = sceneMaterials[i].vKs.x();
        materials[i].vKs.s[1] = sceneMaterials[i].vKs.y();
        materials[i].vKs.s[2] = sceneMaterials[i].vKs.z();
        materials[i].vKs.s[3] = sceneMaterials[i].vKs.w();
        
        materials[i].fEs = sceneMaterials[i].fEs;
        materials[i].uTd = sceneMaterials[i].uTd;
    }

    materialBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(DevMaterialRep) * materials.size(), (void*)&materials[0], &status);
    CHECK_ERROR(status, "Cannot create material buffer");

    textureBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(cl_float) * TEXTURE_BUFFER_SIZE, nullptr, &status);
    CHECK_ERROR(status, "Cannot create texture buffer");

    textureDescBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY, sizeof(DevTextureDesc) * MAX_TEXTURE_HANDLES, nullptr, &status);
    CHECK_ERROR(status, "Cannot create texture handles buffer");

    areaLightsBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * MAX_AREA_LIGHTS, &areaLights[0], &status);
    CHECK_ERROR(status, "Cannot create area lights buffer");
    
    /// new architecture
    pathStartBuffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(DevPathStart) * outputSize_.s[0] * outputSize_.s[1], NULL, &status);
    CHECK_ERROR(status, "Cannot create path start buffer");
    
    taskCounterBuffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_int), NULL, &status);
    CHECK_ERROR(status, "Cannot create task counter buffer");

    bvhIndicesBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * bvh.GetPrimitiveIndexCount(), (void*)bvh.GetPrimitiveIndices(), &status);
    CHECK_ERROR(status, "Cannot create area lights buffer");

    traceShadingData_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(DevShadingData) * width * height, nullptr, &status);
    CHECK_ERROR(status, "Cannot create trace shading data buffer");
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
    configData_.uNumPointLights = 0;
    configData_.uNumRandomNumbers = RANDOM_BUFFER_SIZE;
    configData_.uFrameCount = frameCount_;

    configData_.vBackgroundColor.s[0] = 50.f;
    configData_.vBackgroundColor.s[1] = 47.f;
    configData_.vBackgroundColor.s[2] = 53.1f;
    configData_.vBackgroundColor.s[3] = 1.0f;

    configData_.uNumAreaLights = 2;

    CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, configBuffer_, CL_FALSE, 0, sizeof(configData_), &configData_, 0, nullptr, nullptr), "Cannot update param buffer");

    if (TexturesDirty())
    {
        CompileTextures();
        ResetTexturesDirty();
    }
}

OCLRender::~OCLRender()
{
    /// TODO: implement RAII for CL objects
    glDeleteTextures(1, &glDepthTexture_);

    clReleaseMemObject(traceShadingData_);
    clReleaseMemObject(bvhIndicesBuffer_);
    clReleaseMemObject(taskCounterBuffer_);
    clReleaseMemObject(pathStartBuffer_);
    clReleaseMemObject(areaLightsBuffer_);
    clReleaseMemObject(textureBuffer_);
    clReleaseMemObject(textureDescBuffer_);
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

    clReleaseKernel(traceExperiments_);
    clReleaseKernel(pathShadeAndExportKernel_);
    clReleaseKernel(pathGenerationKernel_);
    clReleaseKernel(pathTraceKernel_);
}

void OCLRender::Render()
{
    cl_event kernelExecutionEvent1, kernelExecutionEvent2, kernelExecutionEvent3, kernelExecutionEvent4 ;

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
    
    // new architecture
    {
        CHECK_ERROR(clSetKernelArg(pathGenerationKernel_, 0, sizeof(cl_mem), &configBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathGenerationKernel_, 1, sizeof(cl_mem), &pathStartBuffer_), "SetKernelArg failed");
        
        cl_int status = clEnqueueNDRangeKernel(commandQueue_, pathGenerationKernel_, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, &kernelExecutionEvent1);
        CHECK_ERROR(status, "Path generation kernel launch failed");

        int numTasks = outputSize_.s[0] * outputSize_.s[1];

        size_t localWorkSize1 = 64;
        size_t globalWorkSize1 = 32 * 40 * localWorkSize1;

        cl_int initialTaskCount = globalWorkSize1;
        CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, taskCounterBuffer_, CL_FALSE, 0, sizeof(cl_int), &initialTaskCount, 0, nullptr, nullptr), "Cannot update task counter buffer");

        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 0, sizeof(cl_mem), &configBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 1, sizeof(cl_mem), &pathStartBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 2, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 3, sizeof(cl_mem), &bvhIndicesBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 4, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 5, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 6, sizeof(cl_mem), &materialBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 7, sizeof(cl_mem), &areaLightsBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 8, sizeof(cl_mem), &textureDescBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 9, sizeof(cl_mem), &textureBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 10, sizeof(cl_mem), &pathBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 11, sizeof(cl_mem), &taskCounterBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 12, sizeof(cl_int), &numTasks), "SetKernelArg failed");
        
        //status = clEnqueueNDRangeKernel(commandQueue_, pathTraceKernel_, 1, nullptr, &globalWorkSize1, &localWorkSize1, 0, nullptr, &kernelExecutionEvent2);
        CHECK_ERROR(status, "Path tracing kernel launch failed");

        CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, taskCounterBuffer_, CL_FALSE, 0, sizeof(cl_int), &initialTaskCount, 0, nullptr, nullptr), "Cannot update task counter buffer");

        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 0, sizeof(cl_mem), &configBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 1, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 2, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 3, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 4, sizeof(cl_mem), &pathStartBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 5, sizeof(cl_mem), &materialBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 6, sizeof(cl_mem), &textureDescBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 7, sizeof(cl_mem), &textureBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 8, sizeof(cl_mem), &areaLightsBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 9, sizeof(cl_mem), &pathBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 10, sizeof(cl_mem), &intermediateBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 11, sizeof(cl_mem), &outputDepthTexture_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 12, sizeof(cl_mem), &taskCounterBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 13, sizeof(cl_int), &numTasks), "SetKernelArg failed");

        //status = clEnqueueNDRangeKernel(commandQueue_, pathShadeAndExportKernel_, 1, nullptr, &globalWorkSize1, &localWorkSize1, 0, nullptr, &kernelExecutionEvent3);
        CHECK_ERROR(status, "Shade kernel launch failed");

        CHECK_ERROR(clSetKernelArg(traceExperiments_, 0, sizeof(cl_mem), &pathStartBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(traceExperiments_, 1, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(traceExperiments_, 2, sizeof(cl_mem), &bvhIndicesBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(traceExperiments_, 3, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(traceExperiments_, 4, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(traceExperiments_, 5, sizeof(cl_mem), &traceShadingData_), "SetKernelArg failed");
        CHECK_ERROR(clSetKernelArg(traceExperiments_, 6, sizeof(cl_mem), &outputDepthTexture_), "SetKernelArg failed");

        localWorkSize1 = 64;
        globalWorkSize1 = configData_.uOutputHeight * configData_.uOutputWidth;
        status = clEnqueueNDRangeKernel(commandQueue_, traceExperiments_, 1, nullptr, &globalWorkSize1, &localWorkSize1, 0, nullptr, &kernelExecutionEvent4);
        CHECK_ERROR(status, "Shade kernel launch failed");
    }

    CHECK_ERROR(clEnqueueReleaseGLObjects(commandQueue_, 1, gl_objects, 0,0,0), "Cannot release OpenGL objects");

    CHECK_ERROR(clFinish(commandQueue_), "Cannot flush command queue");

    //CHECK_ERROR(clWaitForEvents(1, &kernelExecutionEvent), "Wait for events failed");

    cl_ulong startTime, endTime;
    double totalTime;

    clGetEventProfilingInfo(kernelExecutionEvent1, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    clGetEventProfilingInfo(kernelExecutionEvent1, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    totalTime = (double)(endTime - startTime)/1000000.0;

    std::cout << "Ray generation time " << totalTime << " msec\n";

    //clGetEventProfilingInfo(kernelExecutionEvent2, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    //clGetEventProfilingInfo(kernelExecutionEvent2, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    //totalTime = (double)(endTime - startTime)/1000000.0;

    //std::cout << "Ray tracing time " << totalTime << " msec\n";

    //clGetEventProfilingInfo(kernelExecutionEvent3, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    //clGetEventProfilingInfo(kernelExecutionEvent3, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    //totalTime = (double)(endTime - startTime)/1000000.0;

    clGetEventProfilingInfo(kernelExecutionEvent4, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    clGetEventProfilingInfo(kernelExecutionEvent4, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    totalTime = (double)(endTime - startTime)/1000000.0;

    std::cout << "Primary time " << totalTime << " msec\n";
    ++frameCount_;
}

void   OCLRender::FlushFrame()
{
    frameCount_ = 0;
}

void OCLRender::CompileTextures()
{
    cl_int status = CL_SUCCESS;
    std::vector<DevTextureDesc> textureDescs;

    float* data = (float*)clEnqueueMapBuffer(commandQueue_, textureBuffer_, CL_TRUE, CL_MAP_WRITE, 0, TEXTURE_BUFFER_SIZE * sizeof(cl_float), 0, nullptr, nullptr, &status);
    CHECK_ERROR(status, "Cannot map buffer");

    unsigned offset = 0;

    for (auto iter = TexturesCBegin(); iter != TexturesCEnd(); ++iter)
    {
        DevTextureDesc desc;
        desc.uWidth = iter->second->GetWidth();
        desc.uHeight = iter->second->GetHeight();
        desc.uPoolOffset = offset;

        float const* texData = iter->second->GetData();

        unsigned texDataSize = desc.uWidth * desc.uHeight * 4;
        for (int i = 0; i < texDataSize; ++i)
        {
            data[offset + i] = texData[i];
        }

        offset += desc.uWidth * desc.uHeight;

        textureDescs.push_back(desc);
    }

    CHECK_ERROR(clEnqueueUnmapMemObject(commandQueue_, textureBuffer_, data, 0, nullptr, nullptr), "Cannot unmap buffer");

    CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, textureDescBuffer_, CL_TRUE, 0, sizeof(DevTextureDesc) * textureDescs.size(), &textureDescs[0], 0, nullptr, nullptr), "Cannot update texture desc buffer");

    configData_.uTextureCount = static_cast<unsigned>(textureDescs.size());

}





