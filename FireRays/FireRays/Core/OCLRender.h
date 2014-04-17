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
	void		Render();
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


    CLWDevice  device_;
    CLWContext context_;
    CLWProgram program_;

    CLWBuffer<DevPathVertex> firstHitBuffer_;
    CLWBuffer<DevVertex>	vertexBuffer_;
    CLWBuffer<cl_uint4>		indexBuffer_;
    CLWBuffer<DevBVHNode>	bvhBuffer_;
    CLWBuffer<DevConfig>	configBuffer_;
    CLWImage2D	            outputDepthTexture_;
    CLWBuffer<cl_float4>     intermediateBuffer_;


    CLWBuffer<DevMaterialRep>  materialBuffer_;
    CLWBuffer<cl_float>       textureBuffer_;
    CLWBuffer<DevTextureDesc>  textureDescBuffer_;
    CLWBuffer<cl_uint>         areaLightsBuffer_;
    CLWBuffer<uint>            bvhIndicesBuffer_;

    CLWBuffer<DevPathStart> pathStartBuffer_;

	GLuint		glDepthTexture_;

	cl_uint2  outputSize_;
	cl_uint   frameCount_;

	DevConfig configData_;
};

#endif // OCLRENDER_H

