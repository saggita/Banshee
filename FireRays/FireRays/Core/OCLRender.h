//
//  OCLRender.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef OCLRENDER_H
#define OCLRENDER_H

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include "GL/glew.h"
#include "GLUT/GLUT.h"
#endif

#include "../../CLW/CLW.h"
#include "Common.h"
#include "RenderBase.h"
#include "CommonTypes.h"

class TextureBase;
class OCLRender : public RenderBase
{
public:
	OCLRender(cl_platform_id platform);
	~OCLRender();

	void		Init(unsigned width, unsigned height);
	void		Render(float timeDeltaSecs);
	void		Commit();
    
	unsigned	GetOutputTexture() const;
	void		FlushFrame();

private:
    
    void CompileTextures();

	struct __declspec(align(16)) DevConfig
	{
        cl_float4 vBackgroundColor;
        
		cl_float3 vCameraDir;
		cl_float3 vCameraRight;
		cl_float3 vCameraUp;
		cl_float3 vCameraPos;

		cl_float fCameraNearZ;
		cl_float fCameraPixelSize;

		cl_uint uOutputWidth;
		cl_uint uOutputHeight;

		cl_uint uNumPointLights;
		cl_uint uNumAreaLights;
		cl_uint uNumRandomNumbers;
		cl_uint uFrameCount;
        cl_uint uTextureCount;
	};

	struct __declspec(align(16)) DevBVHNode
	{
		struct
		{
			cl_float3 vMin;
			cl_float3 vMax;
		} sBox;

		cl_uint uPrimStart;
		cl_uint uRight;
		cl_uint uParent;
		cl_uint uPrimCount;
	};

	struct __declspec(align(16)) sBox
	{
		cl_float3 vMin;
		cl_float3 vMax;
	};


	struct __declspec(align(16)) DevVertex
	{
		cl_float4 vPos;
		cl_float4 vNormal;
		cl_float2 vTex;
	};

	struct __declspec(align(16)) DevPointLight
	{
		cl_float4 vPos;
		cl_float4 vColor;
		cl_float4 vAttenuation;
	};

	struct  __declspec(align(16)) DevShadingData
	{
		cl_float3 vPos;
		cl_float3 vNormal;
		cl_float2 vTex;
		cl_uint   uMaterialIdx;
        cl_bool   bHit;
	};

	struct  __declspec(align(16)) DevPathVertex
	{
		DevShadingData sShadingData;
		cl_float3 vIncidentDir;
		cl_float4 vRadiance;
		cl_uint	  uMaterialIdx;
        cl_int    iPixelRef;
        cl_float  fDistance;
        cl_bool   bHit;
	};

	struct  __declspec(align(16)) DevMaterialRep
	{
		cl_float4 vKe;
		cl_float4 vKd;
		cl_float4 vKs;
        cl_float  fEs;
		cl_uint   eBsdf;
        cl_uint   uTd;
	} ;
    
    struct __declspec(align(16)) DevTextureDesc
    {
        cl_uint uWidth;
        cl_uint uHeight;
        cl_uint uPoolOffset;
    };
    
    /// new architecture
    struct __declspec(align(16)) DevRay
    {
        cl_float3 vO;
        cl_float3 vD;
        cl_float  fMinT;
        cl_float  fMaxT;
    };
    
    struct __declspec(align(16)) DevPathStart
    {
        DevRay sRay;
        cl_int    iId;
        cl_uint   uPathLength;
        cl_uint   uPathOffset;
        cl_uint   uLastMiss;
        cl_uint   ePathType;
    };

    struct __declspec(align(16)) DevPathExtensionRequest
    {
        DevRay  sRay;
        cl_uint uData;
    };

    CLWDevice  device_;
    CLWContext context_;
    CLWProgram program_;
    CLWParallelPrimitives prims_;

    // Stores first hit information
    CLWBuffer<DevPathVertex> firstHitBuffer_;
    // Stores second hit information
    CLWBuffer<DevPathVertex> secondHitBuffer_;
    // Stores second hit information
    CLWBuffer<DevPathExtensionRequest> secondaryRaysBuffer_;
    // Stores scene geometry
    CLWBuffer<DevVertex>	vertexBuffer_;
    // Stores scene index data
    CLWBuffer<cl_uint4>		indexBuffer_;
    // Stores BVH nodes
    CLWBuffer<DevBVHNode>	bvhBuffer_;
    // Stores current render configuration
    CLWBuffer<DevConfig>	configBuffer_;
    // Texture to output raytraced image
    CLWImage2D	            outputDepthTexture_;
    // Intermediate buffer for samples accumulation
    CLWBuffer<cl_float4>     radianceBuffer_;
    // Intermediate buffer for samples accumulation
    CLWBuffer<cl_float4>     radianceBuffer1_;
    // Log luminance buffer for tone-mapping
    CLWBuffer<cl_float>      logLumBuffer_;
    // Stores hit predicates for a compact operation
    CLWBuffer<cl_int>        hitPredicateBuffer_;
    // Stores sample predicates for a compact operation
    CLWBuffer<cl_int>        samplePredicateBuffer_;
    // Stores initial path indices
    CLWBuffer<cl_int>        initialPathIndicesBuffer_;
    // Stores compacted path indices;
    CLWBuffer<cl_int>        compactedPathIndicesBuffer_;

    // Stores material representations for the scene
    CLWBuffer<DevMaterialRep>  materialBuffer_;
    // Stores texture data for the scene
    CLWBuffer<cl_float>        textureBuffer_;
    // Stores texture descriptions
    CLWBuffer<DevTextureDesc>  textureDescBuffer_;
    // Stores area light indices
    CLWBuffer<cl_uint>         areaLightsBuffer_;
    // Stores BVH intermediate indices
    CLWBuffer<uint>            bvhIndicesBuffer_;

    // Stores path start information
    CLWBuffer<DevPathExtensionRequest> pathStartBuffer_;

    // Framebuffer size
    cl_uint2  outputSize_;

    // Frame count
    cl_uint   frameCount_;

    // Render configuration on the host side
    DevConfig configData_;

    // OpenGL texture for interop
    GLuint glDepthTexture_;
};

#endif // OCLRENDER_H

