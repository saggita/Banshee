//
//  OCLRender.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__opencl_render__
#define __BVHOQ__opencl_render__

#include <iostream>

//
//  rt_cpu_gold.h
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_opencl_render_h
#define BVHOQ_opencl_render_h

#include <vector>
#include <memory>

#ifdef WIN32
#include <gl/glew.h>
#endif
#include <GLUT/GLUT.h>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#else
#include <CL/cl.h>
#include <CL/cl_gl.h>
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
	
	void Init(unsigned width, unsigned height);
	void Render();
	void Commit();

	GLuint GetOutputTexture() const;
    void   FlushFrame();

private:
	struct __declspec(align(1)) DevConfig
	{
		cl_float16 mProjInv;
		cl_float16 mView;

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
	
	struct __declspec(align(1)) DevBVHNode
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

	struct  __declspec(align(1)) DevDrawCommand
	{
		cl_uint  uCount;
		cl_uint  uInstanceCount;
		cl_uint  uFirstIndex;
		cl_uint  uBaseVertex;
		cl_uint  uBaseInstance;
	};

	struct  __declspec(align(1)) DevOffsets
	{
		cl_uint  uStartIdx;
		cl_uint  uNumIndices;
	};
    
    struct __declspec(align(1)) DevVertex
    {
        cl_float4 vPos;
        cl_float4 vNormal;
        cl_float2 vTex;
    };
    
    struct __declspec(align(1)) DevPointLight
    {
        cl_float4 vPos;
        cl_float4 vColor;
        cl_float4 vAttenuation;
    };
    
    struct DevShadingData
    {
        cl_float3 vPos;
        cl_float3 vNormal;
        cl_float2 vTex;
        cl_uint   uMaterialIdx;
    };
    
    struct DevPathVertex
    {
        DevShadingData shadingData;
        cl_float3 vIncidentDir;
        cl_float3 vRadiance;
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
	
	GLuint    glDepthTexture_;

	cl_uint2  outputSize_;
    cl_uint   frameCount_;
	
	DevConfig configData_;

	std::shared_ptr<BVHAccelerator> accel_;
};

#endif


#endif /* defined(__BVHOQ__opencl_render__) */
