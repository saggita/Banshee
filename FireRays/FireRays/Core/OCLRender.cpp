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
    : frameCount_(0)
{

}

void OCLRender::Init(unsigned width, unsigned height)
{
    outputSize_.s[0] = width;
    outputSize_.s[1] = height;

    std::vector<CLWPlatform> platforms;
    CLWPlatform::CreateAllPlatforms(platforms);

    cl_platform_id platformId = platforms[0];

#ifdef __APPLE__
    CGLContextObj kCGLContext = CGLGetCurrentContext(); // GL Context
    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext); // Share Group
    cl_context_properties props[] =
    {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties) kCGLShareGroup, CL_CONTEXT_PLATFORM,
        (cl_context_properties) platformId,
        0
    };
#elif WIN32
    cl_context_properties props[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR,
        (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties)platformId, 0
    };
#endif

    device_ = platforms[0].GetDevice(0);
    context_ = CLWContext::Create(device_, props);

    std::vector<char> sourceCode;
    LoadFileContents("../../../FireRays/CL/raytrace.cl", sourceCode);

    program_ = context_.CreateProgram(sourceCode); 

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

    vertexBuffer_ = context_.CreateBuffer<DevVertex>(vertices.size(), &vertices[0]);

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

    indexBuffer_ = context_.CreateBuffer<cl_uint4>(triangles.size(), &triangles[0]);

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

    bvhBuffer_ = context_.CreateBuffer<DevBVHNode>(nodes.size(), &nodes[0]);

    configBuffer_ = context_.CreateBuffer<DevConfig>(1);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &glDepthTexture_);

    glBindTexture(GL_TEXTURE_2D, glDepthTexture_);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, outputSize_.s[0], outputSize_.s[1], 0, GL_RGBA, GL_FLOAT, nullptr);

    outputDepthTexture_ = context_.CreateImage2DFromGLTexture(glDepthTexture_);

    glBindTexture(GL_TEXTURE_2D, 0);


    intermediateBuffer_ = context_.CreateBuffer<cl_float4>(outputSize_.s[0] * outputSize_.s[1]);

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

    materialBuffer_     = context_.CreateBuffer<DevMaterialRep>(materials.size(), &materials[0]);
    textureBuffer_      = context_.CreateBuffer<cl_float>(TEXTURE_BUFFER_SIZE);
    textureDescBuffer_  = context_.CreateBuffer<DevTextureDesc>(MAX_TEXTURE_HANDLES);
    areaLightsBuffer_   = context_.CreateBuffer<cl_uint>(MAX_AREA_LIGHTS, &areaLights[0]);
    pathStartBuffer_    = context_.CreateBuffer<DevPathStart>(outputSize_.s[0] * outputSize_.s[1]);
    bvhIndicesBuffer_   = context_.CreateBuffer<cl_uint>(bvh.GetPrimitiveIndexCount(), (void*)bvh.GetPrimitiveIndices());
    firstHitBuffer_     = context_.CreateBuffer<DevPathVertex>(outputSize_.s[0] * outputSize_.s[1]);
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

    context_.WriteBuffer(0, configBuffer_, &configData_, 1).Wait();

    if (TexturesDirty())
    {
        CompileTextures();
        ResetTexturesDirty();
    }
}

OCLRender::~OCLRender()
{
    glDeleteTextures(1, &glDepthTexture_);
}

void OCLRender::Render()
{
    cl_event kernelExecutionEvent1, kernelExecutionEvent2, kernelExecutionEvent3, kernelExecutionEvent4, kernelExecutionEvent5;

    glFinish();
   
    std::vector<cl_mem> glObjects;
    glObjects.push_back(outputDepthTexture_);

    context_.AcquireGLObjects(0, glObjects);

    size_t localWorkSize[2];

    if (device_.GetType() == CL_DEVICE_TYPE_CPU)
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
        CLWKernel pathGenerationKernel = program_.GetKernel("GeneratePath");
        pathGenerationKernel.SetArg(0, configBuffer_);
        pathGenerationKernel.SetArg(1, pathStartBuffer_);

        context_.Launch2D(0, globalWorkSize, localWorkSize, pathGenerationKernel); 

        //std::vector<DevPathStart> hPathStart(outputSize_.s[0] * outputSize_.s[1]);
        //context_.ReadBuffer(0, pathStartBuffer_, &hPathStart[0], hPathStart.size()).Wait();

        //cl_int status = clEnqueueNDRangeKernel(commandQueue_, pathGenerationKernel_, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, &kernelExecutionEvent1);
        //CHECK_ERROR(status, "Path generation kernel launch failed");
        
        int numTasks = outputSize_.s[0] * outputSize_.s[1];

        size_t localWorkSize1 = 64;
        size_t globalWorkSize1 = 32 * 40 * localWorkSize1;

        //cl_int initialTaskCount = globalWorkSize1;
        //CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, taskCounterBuffer_, CL_FALSE, 0, sizeof(cl_int), &initialTaskCount, 0, nullptr, nullptr), "Cannot update task counter buffer");

        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 0, sizeof(cl_mem), &configBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 1, sizeof(cl_mem), &pathStartBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 2, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 3, sizeof(cl_mem), &bvhIndicesBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 4, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 5, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 6, sizeof(cl_mem), &materialBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 7, sizeof(cl_mem), &areaLightsBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 8, sizeof(cl_mem), &textureDescBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 9, sizeof(cl_mem), &textureBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 10, sizeof(cl_mem), &pathBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 11, sizeof(cl_mem), &taskCounterBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathTraceKernel_, 12, sizeof(cl_int), &numTasks), "SetKernelArg failed");
        
        //status = clEnqueueNDRangeKernel(commandQueue_, pathTraceKernel_, 1, nullptr, &globalWorkSize1, &localWorkSize1, 0, nullptr, &kernelExecutionEvent2);
        //CHECK_ERROR(status, "Path tracing kernel launch failed");

        //CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, taskCounterBuffer_, CL_FALSE, 0, sizeof(cl_int), &initialTaskCount, 0, nullptr, nullptr), "Cannot update task counter buffer");

        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 0, sizeof(cl_mem), &configBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 1, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 2, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 3, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 4, sizeof(cl_mem), &pathStartBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 5, sizeof(cl_mem), &materialBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 6, sizeof(cl_mem), &textureDescBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 7, sizeof(cl_mem), &textureBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 8, sizeof(cl_mem), &areaLightsBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 9, sizeof(cl_mem), &pathBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 10, sizeof(cl_mem), &intermediateBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 11, sizeof(cl_mem), &outputDepthTexture_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 12, sizeof(cl_mem), &taskCounterBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(pathShadeAndExportKernel_, 13, sizeof(cl_int), &numTasks), "SetKernelArg failed");

        //status = clEnqueueNDRangeKernel(commandQueue_, pathShadeAndExportKernel_, 1, nullptr, &globalWorkSize1, &localWorkSize1, 0, nullptr, &kernelExecutionEvent3);
        //CHECK_ERROR(status, "Shade kernel launch failed");

        //CHECK_ERROR(clSetKernelArg(traceExperiments_, 0, sizeof(cl_mem), &pathStartBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(traceExperiments_, 1, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(traceExperiments_, 2, sizeof(cl_mem), &bvhIndicesBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(traceExperiments_, 3, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(traceExperiments_, 4, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(traceExperiments_, 5, sizeof(cl_mem), &firstHitBuffer_), "SetKernelArg failed");

        CLWKernel firstHitKernel = program_.GetKernel("TraceExperiments");
        firstHitKernel.SetArg(0, pathStartBuffer_);
        firstHitKernel.SetArg(1, bvhBuffer_);
        firstHitKernel.SetArg(2, bvhIndicesBuffer_);
        firstHitKernel.SetArg(3, vertexBuffer_);
        firstHitKernel.SetArg(4, indexBuffer_);
        firstHitKernel.SetArg(5, firstHitBuffer_);

        localWorkSize1 = 64;
        globalWorkSize1 = configData_.uOutputHeight * configData_.uOutputWidth;
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, firstHitKernel);

        CLWKernel directIlluminationKernel = program_.GetKernel("DirectIllumination");

        directIlluminationKernel.SetArg(0, bvhBuffer_);
        directIlluminationKernel.SetArg(1, bvhIndicesBuffer_);
        directIlluminationKernel.SetArg(2, vertexBuffer_);
        directIlluminationKernel.SetArg(3, indexBuffer_);
        directIlluminationKernel.SetArg(4, firstHitBuffer_);
        directIlluminationKernel.SetArg(5, materialBuffer_);
        directIlluminationKernel.SetArg(6, textureDescBuffer_);
        directIlluminationKernel.SetArg(7, textureBuffer_);
        directIlluminationKernel.SetArg(8, outputDepthTexture_);
        directIlluminationKernel.SetArg(9, intermediateBuffer_);
        directIlluminationKernel.SetArg(10, frameCount_);

        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 0, sizeof(cl_mem), &bvhBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 1, sizeof(cl_mem), &bvhIndicesBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 2, sizeof(cl_mem), &vertexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 3, sizeof(cl_mem), &indexBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 4, sizeof(cl_mem), &firstHitBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 5, sizeof(cl_mem), &materialBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 6, sizeof(cl_mem), &textureDescBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 7, sizeof(cl_mem), &textureBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 8, sizeof(cl_mem), &outputDepthTexture_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 9, sizeof(cl_mem), &intermediateBuffer_), "SetKernelArg failed");
        //CHECK_ERROR(clSetKernelArg(directIlluminationKernel_, 10, sizeof(cl_uint), &frameCount_), "SetKernelArg failed");
        //status = clEnqueueNDRangeKernel(commandQueue_, directIlluminationKernel_, 1, nullptr, &globalWorkSize1, &localWorkSize1, 0, nullptr, &kernelExecutionEvent5);
        //CHECK_ERROR(status, "Shade kernel launch failed");
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, directIlluminationKernel);
    }

    context_.ReleaseGLObjects(0, glObjects);

    //CHECK_ERROR(clEnqueueReleaseGLObjects(commandQueue_, 1, gl_objects, 0,0,0), "Cannot release OpenGL objects");

    //CHECK_ERROR(clFinish(commandQueue_), "Cannot flush command queue");

    //CHECK_ERROR(clWaitForEvents(1, &kernelExecutionEvent), "Wait for events failed");

    //cl_ulong startTime, endTime;
    //double totalTime;

    //clGetEventProfilingInfo(kernelExecutionEvent1, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    //clGetEventProfilingInfo(kernelExecutionEvent1, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    //totalTime = (double)(endTime - startTime)/1000000.0;

    //std::cout << "Ray generation time " << totalTime << " msec\n";

    //clGetEventProfilingInfo(kernelExecutionEvent2, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    //clGetEventProfilingInfo(kernelExecutionEvent2, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    //totalTime = (double)(endTime - startTime)/1000000.0;

    //std::cout << "Ray tracing time " << totalTime << " msec\n";

    //clGetEventProfilingInfo(kernelExecutionEvent3, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    //clGetEventProfilingInfo(kernelExecutionEvent3, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    //totalTime = (double)(endTime - startTime)/1000000.0;

    //clGetEventProfilingInfo(kernelExecutionEvent4, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    //clGetEventProfilingInfo(kernelExecutionEvent4, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    //totalTime = (double)(endTime - startTime)/1000000.0;

    //std::cout << "Primary path tracing " << totalTime << " msec\n";

    //clGetEventProfilingInfo(kernelExecutionEvent5, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    //clGetEventProfilingInfo(kernelExecutionEvent5, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    //totalTime = (double)(endTime - startTime)/1000000.0;

    //std::cout << "Shading time " << totalTime << " msec\n";

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

    float* data = nullptr;
    context_.MapBuffer(0, textureBuffer_, &data).Wait();


    //(float*)clEnqueueMapBuffer(commandQueue_, textureBuffer_, CL_TRUE, CL_MAP_WRITE, 0, TEXTURE_BUFFER_SIZE * sizeof(cl_float), 0, nullptr, nullptr, &status);
    //CHECK_ERROR(status, "Cannot map buffer");

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

    context_.UnmapBuffer(0, textureBuffer_, data);

    //CHECK_ERROR(clEnqueueUnmapMemObject(commandQueue_, textureBuffer_, data, 0, nullptr, nullptr), "Cannot unmap buffer");

    //CHECK_ERROR(clEnqueueWriteBuffer(commandQueue_, textureDescBuffer_, CL_TRUE, 0, sizeof(DevTextureDesc) * textureDescs.size(), &textureDescs[0], 0, nullptr, nullptr), "Cannot update texture desc buffer");
    context_.WriteBuffer(0, textureDescBuffer_, &textureDescs[0], textureDescs.size()).Wait();

    configData_.uTextureCount = static_cast<unsigned>(textureDescs.size());

}





