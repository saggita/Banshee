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
    
    cl_platform_id platformId = platforms[1];
    
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
    
    device_ = platforms[1].GetDevice(0);
    context_ = CLWContext::Create(device_, props);
    prims_ = CLWParallelPrimitives(context_);
    
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
    
    vertexBuffer_ = context_.CreateBuffer<DevVertex>(vertices.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &vertices[0]);


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
    
    std::vector<cl_uint4> triangles(builder.GetPrimitiveCount());
    std::vector<cl_uint>  areaLights;
    
    SplitBVHBuilder::Primitive const* sceneTriangles = builder.GetPrimitives();
    for (int i = 0; i < builder.GetPrimitiveCount(); ++i)
    {
        triangles[i].s[0] = sceneTriangles[i].i1;
        triangles[i].s[1] = sceneTriangles[i].i2;
        triangles[i].s[2] = sceneTriangles[i].i3;
        triangles[i].s[3] = sceneTriangles[i].m;
        
        if (materials[sceneTriangles[i].m].eBsdf == 3)
            areaLights.push_back(i);
    }
    
    indexBuffer_ = context_.CreateBuffer<cl_uint4>(triangles.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &triangles[0]);
    indexBufferReordered_ = context_.CreateBuffer<cl_uint4>(bvh.GetPrimitiveIndexCount(), CL_MEM_READ_WRITE);
    
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
        nodes[i].uNext    = accelNodes[i].next;
        nodes[i].uPrimCount = accelNodes[i].primCount;
    }
    
    bvhBuffer_ = context_.CreateBuffer<DevBVHNode>(nodes.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &nodes[0]);
    
    configBuffer_ = context_.CreateBuffer<DevConfig>(1, CL_MEM_READ_WRITE);
    
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

    radianceBuffer_  = context_.CreateBuffer<cl_float4>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    radianceBuffer1_ = context_.CreateBuffer<cl_float4>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    
    
    materialBuffer_     = context_.CreateBuffer<DevMaterialRep>(materials.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &materials[0]);
    textureBuffer_      = context_.CreateBuffer<cl_float>(TEXTURE_BUFFER_SIZE, CL_MEM_READ_WRITE);
    textureDescBuffer_  = context_.CreateBuffer<DevTextureDesc>(MAX_TEXTURE_HANDLES, CL_MEM_READ_WRITE);
    
    areaLightsBuffer_   = context_.CreateBuffer<cl_uint>(MAX_AREA_LIGHTS, CL_MEM_READ_WRITE);
    pathStartBuffer_    = context_.CreateBuffer<DevPathExtensionRequest>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    bvhIndicesBuffer_   = context_.CreateBuffer<cl_uint>(bvh.GetPrimitiveIndexCount(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)bvh.GetPrimitiveIndices());
    firstHitBuffer_     = context_.CreateBuffer<DevPathVertex>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    secondHitBuffer_    = context_.CreateBuffer<DevPathVertex>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    secondaryRaysBuffer_     = context_.CreateBuffer<DevPathExtensionRequest>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    hitPredicateBuffer_ = context_.CreateBuffer<cl_int>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    samplePredicateBuffer_ = context_.CreateBuffer<cl_int>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    logLumBuffer_ = context_.CreateBuffer<cl_float>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);
    areaLightsCount_ = context_.CreateBuffer<cl_int>(1, CL_MEM_READ_WRITE);

    std::vector<cl_int> initialIndices(outputSize_.s[0] * outputSize_.s[1]);
    for (int i = 0; i < outputSize_.s[0] * outputSize_.s[1]; ++i)
        initialIndices[i] = i;

    initialPathIndicesBuffer_  = context_.CreateBuffer<cl_int>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &initialIndices[0]);
    compactedPathIndicesBuffer_ = context_.CreateBuffer<cl_int>(outputSize_.s[0] * outputSize_.s[1], CL_MEM_READ_WRITE);

    configData_.uNumAreaLights = areaLights.size();
    
    
    /// Reorder primitive indices accordingly to bvh nodes
    size_t numIndices = bvhIndicesBuffer_.GetElementCount();
    
    size_t localWorkSize = 64;
    size_t globalWorkSize = localWorkSize * ((numIndices + localWorkSize - 1) / localWorkSize);
    
    context_.FillBuffer(0, areaLightsCount_, 0, 1).Wait();
    
    CLWKernel reorderBvhIndicesKernel = program_.GetKernel("reorder_bvh_indices");
    reorderBvhIndicesKernel.SetArg(0, indexBuffer_);
    reorderBvhIndicesKernel.SetArg(1, bvhIndicesBuffer_);
    reorderBvhIndicesKernel.SetArg(2, materialBuffer_);
    reorderBvhIndicesKernel.SetArg(3, (cl_uint)numIndices);
    reorderBvhIndicesKernel.SetArg(4, indexBufferReordered_);
    reorderBvhIndicesKernel.SetArg(5, areaLightsBuffer_);
    reorderBvhIndicesKernel.SetArg(6, areaLightsCount_);
    

    context_.Launch1D(0, globalWorkSize, localWorkSize, reorderBvhIndicesKernel);
    
    cl_int numAreaLights;
    context_.ReadBuffer(0, areaLightsCount_, &numAreaLights, 1).Wait();
    
    std::cout << "Num area lights: " << numAreaLights << "\n";
    
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

    configData_.vBackgroundColor.s[0] = 5.f;
    configData_.vBackgroundColor.s[1] = 4.f;
    configData_.vBackgroundColor.s[2] = 5.1f;
    configData_.vBackgroundColor.s[3] = 1.0f;
    
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

void OCLRender::Render(float timeDeltaDesc)
{
    // Initialize work sizes
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

    // PART I: Path generation kernel launch
    // The kernel is supposed to fill initial extension request buffer
    // based on camera parameters
    {
        CLWKernel pathGenerationKernel = program_.GetKernel("generate_rays");
        pathGenerationKernel.SetArg(0, configBuffer_);
        pathGenerationKernel.SetArg(1, pathStartBuffer_);

        context_.Launch2D(0, globalWorkSize, localWorkSize, pathGenerationKernel);
    }

    size_t localWorkSize1 = 64;
    size_t globalWorkSize1 = localWorkSize1 * ((configData_.uOutputHeight * configData_.uOutputWidth + localWorkSize1 - 1) / localWorkSize1);

    // PART II : Extension kernel launch
    // Extension kernel is launched to process extension requests buffer, traces rays against BVH
    // and fills in hit information into hits buffer along with hit predicate 
    // information which might be used to compact away missing rays
    CLWEvent primaryRaysEvent;
    {
        CLWKernel extensionKernel = program_.GetKernel("process_extension_request");
        extensionKernel.SetArg(0, pathStartBuffer_);
        extensionKernel.SetArg(1, initialPathIndicesBuffer_);
        extensionKernel.SetArg(2, bvhBuffer_);
        extensionKernel.SetArg(3, vertexBuffer_);
        extensionKernel.SetArg(4, indexBufferReordered_);
        extensionKernel.SetArg(5, firstHitBuffer_);
        extensionKernel.SetArg(6, hitPredicateBuffer_);
        extensionKernel.SetArg(7, (cl_uint)(configData_.uOutputHeight * configData_.uOutputWidth));

        primaryRaysEvent = context_.Launch1D(0, globalWorkSize1, localWorkSize1, extensionKernel);
    }

    // PART III: Compaction
    // To avoid thread divergence on further phases we 
    // compact hits buffer throwing away unactive rays and collecting
    // active ray indices into a separate buffer
    cl_int numActiveRays = 0;
    prims_.Compact(0, hitPredicateBuffer_, initialPathIndicesBuffer_, compactedPathIndicesBuffer_, numActiveRays).Wait();

    //std::cout << "\n" << ((float)numActiveRays/(configData_.uOutputHeight * configData_.uOutputWidth)) * 100 << "% active rays"; 

    /// PART IV: Direct illumination
    /// Calculate direct illumination using hit information along with active ray indices
    /// to fill radiance buffer
    {
        CLWKernel directIlluminationKernel = program_.GetKernel("sample_direct_illumination_env");
        directIlluminationKernel.SetArg(0, bvhBuffer_);
        directIlluminationKernel.SetArg(1, vertexBuffer_);
        directIlluminationKernel.SetArg(2, indexBufferReordered_);
        directIlluminationKernel.SetArg(3, firstHitBuffer_);
        directIlluminationKernel.SetArg(4, compactedPathIndicesBuffer_);
        directIlluminationKernel.SetArg(5, materialBuffer_);
        directIlluminationKernel.SetArg(6, textureDescBuffer_);
        directIlluminationKernel.SetArg(7, textureBuffer_);
        directIlluminationKernel.SetArg(8, areaLightsBuffer_);
        directIlluminationKernel.SetArg(9, areaLightsCount_);
        directIlluminationKernel.SetArg(10, radianceBuffer_);
        directIlluminationKernel.SetArg(11, (cl_uint)numActiveRays);
        directIlluminationKernel.SetArg(12, (cl_uint)frameCount_);

        globalWorkSize1 = localWorkSize1 * ((numActiveRays + localWorkSize1 - 1) / localWorkSize1);

        context_.Launch1D(0, globalWorkSize1, localWorkSize1, directIlluminationKernel);
    }

    /// PART V: Materials sampling
    /// Materials sampling kernel is supposed to generate secondary extension requests
    {
        CLWKernel sampleMaterialKernel = program_.GetKernel("sample_material");
        sampleMaterialKernel.SetArg(0, firstHitBuffer_);
        sampleMaterialKernel.SetArg(1, compactedPathIndicesBuffer_);
        sampleMaterialKernel.SetArg(2, materialBuffer_);
        sampleMaterialKernel.SetArg(3, textureDescBuffer_);
        sampleMaterialKernel.SetArg(4, textureBuffer_);
        sampleMaterialKernel.SetArg(5, (cl_uint)numActiveRays);
        sampleMaterialKernel.SetArg(6, (cl_uint)frameCount_);
        sampleMaterialKernel.SetArg(7, secondaryRaysBuffer_);
        sampleMaterialKernel.SetArg(8, samplePredicateBuffer_);

        globalWorkSize1 = localWorkSize1 * ((numActiveRays + localWorkSize1 - 1) / localWorkSize1);

        context_.FillBuffer(0, samplePredicateBuffer_, 0, samplePredicateBuffer_.GetElementCount()).Wait();
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, sampleMaterialKernel);
    }

    // PART VI: Compaction
    // To avoid thread divergence on further phases we 
    // compact hits buffer throwing away unactive secondary rays and collecting
    // active ray indices into a separate buffer
    cl_int numActiveSecondaryRays = 0;
    prims_.Compact(0, samplePredicateBuffer_, initialPathIndicesBuffer_, compactedPathIndicesBuffer_, numActiveSecondaryRays).Wait();
    //std::cout << "\n" << ((float)numActiveSecondaryRays/(configData_.uOutputHeight * configData_.uOutputWidth)) * 100 << "% active secondary rays after sampling";

    // PART VII : Extension kernel launch
    // Extension kernel is launched to process extension requests buffer, traces rays against BVH
    // and fills in hit information into hits buffer along with hit predicate 
    // information which might be used to compact away missing rays
    CLWEvent secondayRaysEvent;
    {
        CLWKernel extensionKernel = program_.GetKernel("process_extension_request");
        extensionKernel.SetArg(0, secondaryRaysBuffer_);
        extensionKernel.SetArg(1, compactedPathIndicesBuffer_);
        extensionKernel.SetArg(2, bvhBuffer_);
        extensionKernel.SetArg(3, vertexBuffer_);
        extensionKernel.SetArg(4, indexBufferReordered_);
        extensionKernel.SetArg(5, secondHitBuffer_);
        extensionKernel.SetArg(6, hitPredicateBuffer_);
        extensionKernel.SetArg(7, (cl_uint)(numActiveSecondaryRays));

        globalWorkSize1 = localWorkSize1 * ((numActiveSecondaryRays + localWorkSize1 - 1) / localWorkSize1);
        context_.FillBuffer(0, hitPredicateBuffer_, 0, hitPredicateBuffer_.GetElementCount()).Wait();
        secondayRaysEvent = context_.Launch1D(0, globalWorkSize1, localWorkSize1, extensionKernel);
    }

    // PART VI: Compaction
    // To avoid thread divergence on further phases we 
    // compact hits buffer throwing away inactive secondary rays and collecting
    // active ray indices into a separate buffer
    numActiveRays = 0;
    prims_.Compact(0, hitPredicateBuffer_, initialPathIndicesBuffer_, compactedPathIndicesBuffer_, numActiveRays).Wait();
    //std::cout << "\n" << ((float)numActiveRays/(configData_.uOutputHeight * configData_.uOutputWidth)) * 100 << "% active secondary rays after tracing"; 

    {
        CLWKernel directIlluminationKernel = program_.GetKernel("sample_direct_illumination_env");
        directIlluminationKernel.SetArg(0, bvhBuffer_);
        directIlluminationKernel.SetArg(1, vertexBuffer_);
        directIlluminationKernel.SetArg(2, indexBufferReordered_);
        directIlluminationKernel.SetArg(3, secondHitBuffer_);
        directIlluminationKernel.SetArg(4, compactedPathIndicesBuffer_);
        directIlluminationKernel.SetArg(5, materialBuffer_);
        directIlluminationKernel.SetArg(6, textureDescBuffer_);
        directIlluminationKernel.SetArg(7, textureBuffer_);
        directIlluminationKernel.SetArg(8, areaLightsBuffer_);
        directIlluminationKernel.SetArg(9, areaLightsCount_);
        directIlluminationKernel.SetArg(10, radianceBuffer1_);
        directIlluminationKernel.SetArg(11, (cl_uint)numActiveRays);
        directIlluminationKernel.SetArg(12, (cl_uint)frameCount_);

        cl_float4 zero = { 0.f, 0.f, 0.f, 0.f };
        context_.FillBuffer(0, radianceBuffer1_, zero, radianceBuffer1_.GetElementCount()).Wait();

        globalWorkSize1 = localWorkSize1 * ((numActiveRays + localWorkSize1 - 1) / localWorkSize1);
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, directIlluminationKernel);
    }

    // PART VII: Material evaluation
    // This kernel operates on two radiance and hit buffers and 
    // calculate BRDF for one bounce of illumination
    {
        CLWKernel evalMaterialKernel = program_.GetKernel("evaluate_material");
        evalMaterialKernel.SetArg(0, firstHitBuffer_);
        evalMaterialKernel.SetArg(1, secondHitBuffer_);
        evalMaterialKernel.SetArg(2, compactedPathIndicesBuffer_);
        evalMaterialKernel.SetArg(3, materialBuffer_);
        evalMaterialKernel.SetArg(4, textureDescBuffer_);
        evalMaterialKernel.SetArg(5, textureBuffer_);
        evalMaterialKernel.SetArg(6, radianceBuffer1_);
        evalMaterialKernel.SetArg(7, radianceBuffer_);
        evalMaterialKernel.SetArg(8, (cl_uint)numActiveRays);
        evalMaterialKernel.SetArg(9, (cl_uint)frameCount_);

        globalWorkSize1 = localWorkSize1 * ((numActiveRays + localWorkSize1 - 1) / localWorkSize1);
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, evalMaterialKernel);
    }

    // PART VIII: Post-processing
    // Resolve progressively refined buffer based on the number of accumulated frames
    {
        CLWKernel resolveRadianceKernel = program_.GetKernel("resolve_radiance");
        resolveRadianceKernel.SetArg(0, radianceBuffer_);
        resolveRadianceKernel.SetArg(1, radianceBuffer1_);
        resolveRadianceKernel.SetArg(2, (cl_uint)(configData_.uOutputHeight * configData_.uOutputWidth));
        resolveRadianceKernel.SetArg(3, (cl_uint)(frameCount_));

        globalWorkSize1 = localWorkSize1 * ((configData_.uOutputHeight * configData_.uOutputWidth + localWorkSize1 - 1) / localWorkSize1);
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, resolveRadianceKernel);
    }

    /* USE SCAN FOR NOW: NEED TO SWITCH TO REDUCTION LATER */
    // Log luminance evaluation for tonemapping
    {
        CLWKernel logLumKernel = program_.GetKernel("calculate_loglum");
        logLumKernel.SetArg(0, radianceBuffer1_);
        logLumKernel.SetArg(1, logLumBuffer_);
        logLumKernel.SetArg(2, (cl_uint)(configData_.uOutputHeight * configData_.uOutputWidth));

        globalWorkSize1 = localWorkSize1 * ((configData_.uOutputHeight * configData_.uOutputWidth + localWorkSize1 - 1) / localWorkSize1);
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, logLumKernel);
    }

    prims_.ScanExclusiveAdd(0, logLumBuffer_, logLumBuffer_).Wait();
    cl_float logLum = 0.f;
    context_.ReadBuffer(0, logLumBuffer_, &logLum, logLumBuffer_.GetElementCount() - 1, 1).Wait();

    logLum /= (configData_.uOutputHeight * configData_.uOutputWidth - 1);
    //std::cout << "\nAvg loglum: " << logLum;

    // Tone mapping
    {
        CLWKernel tonemapRadianceKernel = program_.GetKernel("tonemap_radiance");
        tonemapRadianceKernel.SetArg(0, radianceBuffer1_);
        tonemapRadianceKernel.SetArg(1, radianceBuffer1_);
        tonemapRadianceKernel.SetArg(2, (cl_uint)(configData_.uOutputHeight * configData_.uOutputWidth));
        tonemapRadianceKernel.SetArg(3, (cl_float)(logLum));
        tonemapRadianceKernel.SetArg(4, (cl_float)(timeDeltaDesc));

        globalWorkSize1 = localWorkSize1 * ((configData_.uOutputHeight * configData_.uOutputWidth + localWorkSize1 - 1) / localWorkSize1);
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, tonemapRadianceKernel);
    }

    // Export radiance into an OpenGL interop texture
    // Wait for GL commands to finish
    glFinish();

    // Prepare GL objects required for interop
    std::vector<cl_mem> glObjects;
    glObjects.push_back(outputDepthTexture_);

    // Acquire them
    context_.AcquireGLObjects(0, glObjects);

    // Launch export radiance kernel
    {
        CLWKernel exportRadianceKernel = program_.GetKernel("export_radiance");
        exportRadianceKernel.SetArg(0, configBuffer_);
        exportRadianceKernel.SetArg(1, radianceBuffer1_);
        exportRadianceKernel.SetArg(2, outputDepthTexture_);
        exportRadianceKernel.SetArg(3, (cl_uint)(configData_.uOutputHeight * configData_.uOutputWidth));

        globalWorkSize1 = localWorkSize1 * ((configData_.uOutputHeight * configData_.uOutputWidth + localWorkSize1 - 1) / localWorkSize1);
        context_.Launch1D(0, globalWorkSize1, localWorkSize1, exportRadianceKernel);
    }

    context_.Finish(0);

    context_.ReleaseGLObjects(0, glObjects);
    
    primaryRaysEvent.Wait();
    secondayRaysEvent.Wait();
    std::cout << "Primary rays tracing time " << primaryRaysEvent.GetDuration() << " ms\n";
    std::cout << "Secondary rays tracing time " << secondayRaysEvent.GetDuration() << " ms\n";

    ++frameCount_;
}

void   OCLRender::FlushFrame()
{
    frameCount_ = 0;

    // Clear radiance buffer
    cl_float4 zero = { 0.f, 0.f, 0.f, 0.f };
    context_.FillBuffer(0, radianceBuffer_, zero, radianceBuffer_.GetElementCount()).Wait();
    context_.FillBuffer(0, radianceBuffer1_, zero, radianceBuffer1_.GetElementCount()).Wait();
}

void OCLRender::CompileTextures()
{
    std::vector<DevTextureDesc> textureDescs;
    
    float* data = nullptr;
    context_.MapBuffer(0, textureBuffer_, CL_MAP_WRITE, &data).Wait();
    
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
    
    context_.WriteBuffer(0, textureDescBuffer_, &textureDescs[0], textureDescs.size()).Wait();
    
    configData_.uTextureCount = static_cast<unsigned>(textureDescs.size());
}





