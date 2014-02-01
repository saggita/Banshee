//
//  OCLRender.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef OCLRENDER_H
#define OCLRENDER_H

#include <iostream>

#include <vector>
#include <memory>

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

#include "SceneBase.h"
#include "CameraBase.h"
#include "RenderBase.h"
#include "CommonTypes.h"
#include "BVHAccelerator.h"

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

	struct __declspec(align(1)) DevConfig
	{
		cl_float3 vCameraDir;
		cl_float3 vCameraRight;
		cl_float3 vCameraUp;
		cl_float3 vCameraPos;

		cl_float fCameraNearZ;
		cl_float fCameraPixelSize;

		cl_uint uOutputWidth;
		cl_uint uOutputHeight;

		cl_uint uNumPointLights;
		cl_uint uNumRandomNumbers;
		cl_uint uFrameCount;
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

	struct __declspec(align(1)) sBox
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
	};

	struct  __declspec(align(16)) DevPathVertex
	{
		DevShadingData shadingData;
		cl_float3 vIncidentDir;
		cl_float4 vRadiance;
		cl_uint	  uMaterialIdx;
	};

	struct  __declspec(align(16)) DevMaterialRep
	{
		cl_float4 vKd;
		cl_float4 vKs;
		cl_uint   eBsdf;
	} ;
    
    struct __declspec(align(16)) DevTextureDesc
    {
        cl_uint uWidth;
        cl_uint uHeight;
        cl_uint uPoolOffset;
    };


	cl_platform_id platform_;
	cl_device_id   device_;
	cl_device_type deviceType_;
	cl_context	   context_;
	cl_command_queue commandQueue_;
	cl_program       program_;
	cl_kernel        traceDepthKernel_;

	cl_mem		vertexBuffer_;
	cl_mem		indexBuffer_;
	cl_mem		bvhBuffer_;
	cl_mem		configBuffer_;
	cl_mem		outputDepthTexture_;
	cl_mem      pointLights_;
	cl_mem      randomBuffer_;
	cl_mem      pathBuffer_;
	cl_mem      intermediateBuffer_;
	cl_mem		materialBuffer_;

	GLuint		glDepthTexture_;

	cl_uint2  outputSize_;
	cl_uint   frameCount_;

	DevConfig configData_;

	std::shared_ptr<BVHAccelerator> accel_;
};

#endif // OCLRENDER_H

