//
//  Depthmap.cl
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

/*************************************************************************
 EXTENSIONS
**************************************************************************/


/*************************************************************************
 DEFINES
 **************************************************************************/
#define TRAVERSAL_STACKLESS
//#define TRAVERSAL_STACKED
//#define LOCAL_STACK
#define NODE_STACK_SIZE 32
#define TILE_SIZE_X 8
#define TILE_SIZE_Y 8

#define RAY_EPSILON 0.01f
#define NUM_SAMPLES 2.f

#define MATERIAL1 0
#define MATERIAL2 1

#define BSDF_TYPE_LAMBERT  1
#define BSDF_TYPE_SPECULAR 2

#define NULL 0
#define DEFAULT_DEPTH 10000.f;

/*************************************************************************
 TYPE DEFINITIONS
 **************************************************************************/
typedef int BSDF_TYPE;

typedef struct _BBox
    {
        float3 vMin;
        float3 vMax;
    } BBox;

typedef struct _Offset
    {
        uint uStartIdx;
        uint uNumIndices;
    } Offset;

typedef struct _BVHNode
    {
        BBox box;
        // uPrimStartIdx == 0xffffffff for internal nodes
        uint uPrimStartIdx;
        uint uRight;
        uint uParent;
        uint uNumPrims;
    } BVHNode;

typedef struct _Config
    {
        float16 mProjInv;
        float16 mView;
        
        float3 vCameraDir;
        float3 vCameraRight;
        float3 vCameraUp;
        float3 vCameraPos;
        
        float fCameraNearZ;
        float fCameraPixelSize;
        
        uint uOutputWidth;
        uint uOutputHeight;
        
        uint uNumPointLights;
        uint uNumRandomNumbers;
        uint uFrameCount;
        
    } Config;

typedef struct _Ray
    {
        float3 o;
        float3 d;
        float  mint;
        float  maxt;
    } Ray;

typedef struct _Vertex
    {
        float4 vPos;
        float4 vNormal;
        float2 vTex;
    } Vertex;

typedef struct _ShadingData
    {
        float3    vPos;
        float3    vNormal;
        float2    vTex;
        uint      uMaterialIdx;
    } ShadingData;

typedef struct _PathVertex{
        ShadingData sShadingData;
        float3      vIncidentDir;
        float4      vRadiance;
        BSDF_TYPE   eBsdf;
    } PathVertex;

typedef  struct {
	uint  uCount;
	uint  uInstanceCount;
	uint  uFirstIndex;
	uint  uBaseVertex;
	uint  uBaseInstance;
} DrawElementsIndirectCommand;

typedef struct {
    float4 vPos;
    float4 vColor;
    float4 vAttenuation;
} PointLight;

typedef struct {
    __global BVHNode*       sBVH;
    __global Vertex*        sVertices;
    __global uint4*         sIndices;
    __global Config*        sParams;
    __global PointLight*    sPointLights;
} SceneData;

typedef struct {
    __global float* fRands;
    uint            uBufferSize;
    
} RandomBuffer;

typedef struct
{
    uint uVal;
} RNG;

const sampler_t g_sampDefault = CLK_NORMALIZED_COORDS_FALSE |
CLK_ADDRESS_NONE        |
CLK_FILTER_NEAREST;


/*************************************************************************
 HELPER FUNCTIONS
 **************************************************************************/

// Substract float3 vectors
float3 sub(float3 v1, float3 v2)
{
	float3 res;
	res.x = v1.x - v2.x;
	res.y = v1.y - v2.y;
	res.z = v1.z - v2.z;
	return res;
}

uint WangHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

uint RandUint(RNG* sRNG)
{
    sRNG->uVal = 1664525U * sRNG->uVal + 1013904223U;
    return sRNG->uVal;
}

float RandFloat(RNG* sRNG)
{
    return ((float)RandUint(sRNG)) / 0xffffffffU;
}

RNG CreateRNG(uint uSeed)
{
    RNG sRNG;
    sRNG.uVal = WangHash(uSeed);
    for (int i=0;i<100;++i)
        RandFloat(&sRNG);
    return sRNG;
}


float ComponentMask(float3 vVec, int iComponent)
{
	float3 compMask[3] =
	{
		{1.0, 0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 0.0, 1.0}
	};
    
	return dot(vVec, compMask[iComponent]);
}

// Tranform point using 4x4 matrix
float4 TransformPoint(float16 mWVP, float4 vPoint)
{
	float4 vRes;
	vRes.w = mWVP.sc * vPoint.x + mWVP.sd * vPoint.y + mWVP.se * vPoint.z + mWVP.sf * vPoint.w;
	vRes.x = mWVP.s0 * vPoint.x + mWVP.s1 * vPoint.y + mWVP.s2 * vPoint.z + mWVP.s3 * vPoint.w;
	vRes.y = mWVP.s4 * vPoint.x + mWVP.s5 * vPoint.y + mWVP.s6 * vPoint.z + mWVP.s7 * vPoint.w;
	vRes.z = mWVP.s8 * vPoint.x + mWVP.s9 * vPoint.y + mWVP.sa * vPoint.z + mWVP.sb * vPoint.w;
	return vRes;
}


// Intersect Ray against triangle
bool IntersectTriangle(Ray* sRay, float3 vP1, float3 vP2, float3 vP3, float* fA, float* fB)
{
	float3 vE1 = sub(vP2, vP1);
	float3 vE2 = sub(vP3, vP1);
    
	float3 vS1 = cross(sRay->d, vE2);
	float  fInvDir = 1.0/dot(vS1, vE1);
    
	//if (div == 0.f)
	//return false;
    
	float3 vD = sRay->o - vP1;
	float  fB1 = dot(vD, vS1) * fInvDir;
    
	if (fB1 < 0.0 || fB1 > 1.0)
		return false;
    
	float3 vS2 = cross(vD, vE1);
	float  fB2 = dot(sRay->d, vS2) * fInvDir;
    
	if (fB2 < 0.0 || fB1 + fB2 > 1.0)
		return false;
    
	float fTemp = dot(vE2, vS2) * fInvDir;
    
	if (fTemp > 0 && fTemp <= sRay->maxt)
	{
		sRay->maxt = fTemp;
        *fA = fB1;
        *fB = fB2;
		return true;
	}
    
	return false;
}

// Intersect ray with the axis-aligned box
bool IntersectBox(Ray* sRay, BBox sBox)
{
	float3 vRayDir  = make_float3(1.0/sRay->d.x, 1.0/sRay->d.y, 1.0/sRay->d.z);
	float lo = vRayDir.x*(sBox.vMin.x - sRay->o.x);
	float hi = vRayDir.x*(sBox.vMax.x - sRay->o.x);
    
	float tmin = fmin(lo, hi);
	float tmax = fmax(lo, hi);
    
	float lo1 = vRayDir.y*(sBox.vMin.y - sRay->o.y);
	float hi1 = vRayDir.y*(sBox.vMax.y - sRay->o.y);
    
	tmin = fmax(tmin, fmin(lo1, hi1));
	tmax = fmin(tmax, fmax(lo1, hi1));
    
	float lo2 = vRayDir.z*(sBox.vMin.z - sRay->o.z);
	float hi2 = vRayDir.z*(sBox.vMax.z - sRay->o.z);
    
	tmin = fmax(tmin, fmin(lo2, hi2));
	tmax = fmin(tmax, fmax(lo2, hi2));
    
	bool hit = (tmin <= tmax) && (tmax > 0.0);
	float t = tmin >=0 ? tmin : tmax;
    
	if (hit && tmin < sRay->maxt)
	{
		return true;
	}
    
	return false;
}

/*************************************************************************
 BVH FUNCTIONS
 **************************************************************************/
//  intersect a Ray with BVH leaf
bool IntersectLeaf(__global Vertex* vertices, __global uint4* indices, uint idx, int uNumPrims, Ray* r, ShadingData* shadingData)
{
	bool hit = false;
	for (int i = 0; i < uNumPrims; ++i)
	{
		uint4 triangle = indices[idx + i];
        
		float4 v1 = vertices[triangle.x].vPos;
		float4 v2 = vertices[triangle.y].vPos;
		float4 v3 = vertices[triangle.z].vPos;
        
        float4 n1 = vertices[triangle.x].vNormal;
		float4 n2 = vertices[triangle.y].vNormal;
		float4 n3 = vertices[triangle.z].vNormal;
        
        float a = 0;
        float b = 0;
        bool thisHit = IntersectTriangle(r, v1.xyz, v2.xyz, v3.xyz, &a, &b);
        
        if (thisHit)
        {
            shadingData->vPos = (1.f - a - b) * v1.xyz + a * v2.xyz + b * v3.xyz;
            shadingData->vNormal = normalize((1.f - a - b) * n1.xyz + a * n2.xyz + b * n3.xyz);
            shadingData->uMaterialIdx = triangle.w;
        }
        
		hit |= thisHit;
	}
    
	return hit;
}


#define NODE_STACK_PUSH(val) *stack_ptr++ = (val)
#define NODE_STACK_POP *--stack_ptr
#define NODE_STACK_VAL *stack_ptr
#define NODE_STACK_GUARD -1

#ifdef LOCAL_STACK
#define NODE_STACK_INIT(size) \
__local int* stack_ptr = stack; \
*stack_ptr++ = -1
#else
#define NODE_STACK_INIT(size) \
int stack[size]; \
int* stack_ptr = stack;\
*stack_ptr++ = -1
#endif


// intersect Ray against the whole BVH structure
bool TraverseBVHStacked(
#ifdef LOCAL_STACK
                        __local int* stack,
#endif
                        __global BVHNode* nodes, __global Vertex* vertices, __global uint4* indices, Ray* r, ShadingData* shadingData)
{
	// init node stack
	NODE_STACK_INIT(NODE_STACK_SIZE);
    
	bool hit = false;
    
	// start from the root
    uint idx = 0;
	do
	{
		float tt = DEFAULT_DEPTH;
        
		// load current node
		BBox box = nodes[idx].box;
		uint  uLeft  = idx + 1;
		uint  uRight = nodes[idx].uRight;
		uint  uPrimStartIdx = nodes[idx].uPrimStartIdx;
		uint  uNumPrims = nodes[idx].uNumPrims;
        
		// try intersecting against current node's bounding box
		// if this is the leaf try to intersect against contained triangle
		if (uNumPrims != 0)
		{
			hit |= IntersectLeaf(vertices, indices, uPrimStartIdx, uNumPrims, r, shadingData);
		}
		// traverse child nodes otherwise
		else
		{
            BBox lbox = nodes[uLeft].box;
            BBox rbox = nodes[uRight].box;
			bool radd = IntersectBox(r, rbox);
			bool ladd = IntersectBox(r, lbox);
            
			if (radd && ladd)
			{
				int axis = uPrimStartIdx;
				if (ComponentMask(r->d, axis) > 0)
				{
					NODE_STACK_PUSH(uRight);
					idx = uLeft;
				}
				else
				{
					NODE_STACK_PUSH(uLeft);
					idx = uRight;
				}
			}
			else if (radd)
			{
				idx = uRight;
			}
			else if (ladd)
			{
				idx = uLeft;
			}
			else
			{
				idx = NODE_STACK_POP;
			}
            
			continue;
		}
        
		// pop next item from the stack
		idx = NODE_STACK_POP;
	}
	while (idx != NODE_STACK_GUARD);
    
	return hit;
}

#define FROM_PARENT  1
#define FROM_CHILD   2
#define FROM_SIBLING 3

#define uParent(x) (nodes[(x)].uParent)
#define LEFT_CHILD(x) ((x)+1)
#define RIGHT_CHILD(x) (nodes[(x)].uRight)

#define IS_LEAF(x) (nodes[(x)].uNumPrims != 0)

bool TraverseBVHStackless(__global BVHNode* nodes, __global Vertex* vertices, __global uint4* indices, Ray* r, ShadingData* shadingData)
{
	uint current = 1;
	int  state   = FROM_PARENT;
    
	bool hit = false;
    
	while (true)
	{
		switch (state)
		{
            case FROM_CHILD:
			{
				if (current == 0)
				{
					return hit;
				}
                
				else if (current == LEFT_CHILD(uParent(current)))
				{
					current = RIGHT_CHILD(uParent(current));
					state = FROM_SIBLING;
				}
				else
				{
					current = uParent(current);
					state = FROM_CHILD;
				}
				break;
			}
                
            case FROM_SIBLING:
			{
				BBox box = nodes[current].box;
				if (!IntersectBox(r, box))
				{
					current = uParent(current);
					state = FROM_CHILD;
				}
				else if (IS_LEAF(current))
				{
					hit |= IntersectLeaf(vertices, indices, nodes[current].uPrimStartIdx, nodes[current].uNumPrims, r, shadingData);
					current = uParent(current);
					state = FROM_CHILD;
				}
				else
				{
					current = LEFT_CHILD(current);
					state = FROM_PARENT;
				}
                
				break;
			}
                
            case FROM_PARENT:
			{
				BBox box = nodes[current].box;
				if (!IntersectBox(r, box))
				{
					current = RIGHT_CHILD(uParent(current));
					state = FROM_SIBLING;
				}
				else if (IS_LEAF(current))
				{
					hit |= IntersectLeaf(vertices, indices, nodes[current].uPrimStartIdx, nodes[current].uNumPrims, r, shadingData);
					current = RIGHT_CHILD(uParent(current));
					state = FROM_SIBLING;
				}
				else
				{
					current = LEFT_CHILD(current);
					state = FROM_PARENT;
				}
                
				break;
			}
		}
        
	}
    
	return false;
    
}



Ray GetNewRay(ShadingData* shadingData, float3 vIncidentDir)
{
    float3 vReflDir = vIncidentDir - 2*dot(vIncidentDir, shadingData->vNormal) * shadingData->vNormal;
        
    Ray r;
    r.o = shadingData->vPos + RAY_EPSILON * vReflDir;
    r.d = normalize(vReflDir);
    r.mint = 0.f;
    r.maxt = 10000.f;
    
    return r;
}

BSDF_TYPE GetBSDFFromMaterial(uint uMaterialIdx, RNG* sRNG)
{
    switch (uMaterialIdx)
    {
        case MATERIAL2:
        {
            return BSDF_TYPE_LAMBERT;
            
            break;
        }
        case MATERIAL1:
        {
            return RandFloat(sRNG) > 0.3f ? BSDF_TYPE_SPECULAR : BSDF_TYPE_LAMBERT;
            
            break;
    
        }
    }
}

bool SampleBSDF(BSDF_TYPE eBsdf, ShadingData* sShadingData, float3 vWo, RNG* sRNG, Ray* sRay)
{
    switch (eBsdf)
    {
        case BSDF_TYPE_LAMBERT:
        {
            //float fPhi = ((float) RandFloat(sRNG)) * 6.28f;
            //float fPsi = ((float) RandFloat(sRNG)) * 1.71f;
            //float3 vDir = make_float3(sin(fPhi)*sin(fPsi), cos(fPhi)*sin(fPsi), cos(fPsi));
        
            return false;
        }
            
        case BSDF_TYPE_SPECULAR:
        {
            float3 vReflDir = -vWo + 2*dot(vWo, sShadingData->vNormal) * sShadingData->vNormal;
            
            sRay->o = sShadingData->vPos + RAY_EPSILON * vReflDir;
            sRay->d = normalize(vReflDir);
            sRay->mint = 0.f;
            sRay->maxt = 10000.f;
            return true;
        }
    }
}

float4 EvaluateBSDF(BSDF_TYPE eBsdf, ShadingData* sShadingData, float3 vWi, float3 vWo, float4 vRadiance)
{
    switch (eBsdf)
    {
        case BSDF_TYPE_LAMBERT:
            return make_float4(0.2f, 0.3f, 0.3f, 1.f) * vRadiance;
            
        case BSDF_TYPE_SPECULAR:
        {
            float3 vReflDir = -vWi + 2.f*dot(vWi, sShadingData->vNormal) * sShadingData->vNormal;
            return length(vReflDir - vWo) < 0.1f ? vRadiance : make_float4(0.f,0.f,0.f,0.f);
        }
    }
}

float4 SampleDirectIllumination(
                                SceneData*      sSceneData,
                                BSDF_TYPE       eBsdf,
                                ShadingData*    sShadingData,
                                float3          vWo
                                )
{
    float4 vRes = make_float4(0,0,0,1);
    
    for (uint i=0;i<sSceneData->sParams->uNumPointLights;++i)
    {
        float3 vLight = normalize(sSceneData->sPointLights[i].vPos.xyz - sShadingData->vPos);
        
        Ray sRay;
        sRay.o = sShadingData->vPos + RAY_EPSILON * vLight;
        sRay.d = vLight;
        sRay.maxt = length(sSceneData->sPointLights[i].vPos.xyz - sShadingData->vPos);
        sRay.mint = 0.f;
        
        ShadingData sTempData;
        float fShadow = TraverseBVHStackless(sSceneData->sBVH, sSceneData->sVertices, sSceneData->sIndices, &sRay, &sTempData) ? 0.f : 1.f;
        float fNdotL = dot(sShadingData->vNormal, vLight);
        
        vRes += fShadow * EvaluateBSDF(eBsdf, sShadingData, vLight, vWo, sSceneData->sPointLights[i].vColor) * fNdotL;
    }
    
    return vRes;
}

float4 TraceRay(
                SceneData*              sSceneData,
                RNG*                    sRNG,
                Ray*                    sRay,
                __global PathVertex*    sPath,
                int*                    iNumPoolItems
                )
{
    Ray sThisRay = *sRay;
    
    
    ShadingData sShadingData;
    while (TraverseBVHStackless(sSceneData->sBVH, sSceneData->sVertices, sSceneData->sIndices, &sThisRay, &sShadingData))
    {
        PathVertex sNewVertex;
        sNewVertex.vIncidentDir = sThisRay.d;
        sNewVertex.sShadingData = sShadingData;
        sNewVertex.eBsdf = GetBSDFFromMaterial(sShadingData.uMaterialIdx, sRNG);
        
        sNewVertex.vRadiance = SampleDirectIllumination(sSceneData, sNewVertex.eBsdf, &sShadingData, -sThisRay.d);
        
        sPath[*iNumPoolItems] = sNewVertex;
        ++(*iNumPoolItems);
        
        if (*iNumPoolItems >= 3)
            break;
        
        if (!SampleBSDF(sNewVertex.eBsdf, &sShadingData, -sThisRay.d, sRNG, &sThisRay))
        {
            break;
        }
    }
    
    int iPathLength = *iNumPoolItems;
    
    while(*iNumPoolItems > 1)
    {
        float fDirectWeight = 0.3f;
        sPath[*iNumPoolItems - 2].vRadiance *= fDirectWeight;
        
        ShadingData sShadingData = sPath[*iNumPoolItems - 2].sShadingData;
        sPath[*iNumPoolItems - 2].vRadiance += EvaluateBSDF(sPath[*iNumPoolItems - 2].eBsdf, &sShadingData, sPath[*iNumPoolItems - 1].vIncidentDir, -sPath[*iNumPoolItems - 2].vIncidentDir,  sPath[*iNumPoolItems - 1].vRadiance * (1.f - fDirectWeight));
        
        --(*iNumPoolItems);
    }
    
    return iPathLength > 0 ? sPath[0].vRadiance : make_float4(0,0,0,0);
}


/// Raytrace depth kernel
__kernel void TraceDepth(
                         __global BVHNode*      bvh,
                         __global Vertex*       vertices,
                         __global uint4*        indices,
                         __global Config*       params,
                         __global PointLight*   lights,
                         __global float*        rand,
                         __global PathVertex*   pathPool,
                         __read_write image2d_t output
                         )
{
#if defined (TRAVERSAL_PACKET)
	__local int stack[NODE_SHARED_STACK_SIZE];
	__local int flag;
#elif defined (TRAVERSAL_STACKED) && defined (LOCAL_STACK)
	__local int stack[NODE_STACK_SIZE * 64];
#else
#endif
    
	int2 globalId;
	globalId.x = get_global_id(0);
	globalId.y = get_global_id(1);
    
    int flatGlobalId = globalId.y * params->uOutputWidth + globalId.x;
    __global PathVertex* thisThreadPool = pathPool + flatGlobalId;
    
	int2 localId;
	localId.x = get_local_id(0);
	localId.y = get_local_id(1);
    
	int flatLocalId = localId.y * get_local_size(0) + localId.x;
    
	if (globalId.x < params->uOutputWidth && globalId.y < params->uOutputHeight)
	{
		unsigned width  = params->uOutputWidth;
		unsigned height = params->uOutputHeight;
		float pixelSize = params->fCameraPixelSize;
		float nearZ     = params->fCameraNearZ;
        
		int xc = globalId.x - (width >> 1);
		int yc = globalId.y - (height >> 1);
        
        float4 res = make_float4(0,0,0,0);
        RNG sRNG = CreateRNG((get_global_id(0) + 153) * (get_global_id(1) + 173) * (params->uFrameCount + 13));
        
        for (int i = 0; i < NUM_SAMPLES; ++i)
            for (int j = 0; j < NUM_SAMPLES; ++j)
            {
                float fY = yc + (i + RandFloat(&sRNG))/NUM_SAMPLES;
                float fX = xc + (j + RandFloat(&sRNG))/NUM_SAMPLES;
                
                Ray rr;
                rr.o = params->vCameraPos;
                rr.d.x = params->vCameraDir.x * nearZ + params->vCameraUp.x * fY * pixelSize + params->vCameraRight.x * fX * pixelSize;
                rr.d.y = params->vCameraDir.y * nearZ + params->vCameraUp.y * fY * pixelSize + params->vCameraRight.y * fX * pixelSize;
                rr.d.z = params->vCameraDir.z * nearZ + params->vCameraUp.z * fY * pixelSize + params->vCameraRight.z * fX * pixelSize;
                
                rr.d = normalize(rr.d);
                
                rr.mint = 0.f;
                rr.maxt = DEFAULT_DEPTH;
                
                int pathLength = 0;
                
                SceneData sSceneData;
                sSceneData.sBVH         = bvh;
                sSceneData.sVertices    = vertices,
                sSceneData.sIndices     = indices;
                sSceneData.sPointLights = lights;
                sSceneData.sParams      = params;
                
                RandomBuffer sRandBuffer;
                sRandBuffer.fRands       = rand;
                sRandBuffer.uBufferSize  = params->uNumRandomNumbers;
                
                res += TraceRay(&sSceneData, &sRNG, &rr, thisThreadPool, &pathLength);
            }
                                
        res /= (NUM_SAMPLES*NUM_SAMPLES);
        
        uint uFrameCount = params->uFrameCount;
        if (uFrameCount)
        {
            float4 vPrevValue =  read_imagef(output, g_sampDefault, globalId);
            res = vPrevValue * ((float)(uFrameCount - 1.f)/uFrameCount) + res * (1.f/uFrameCount);
        }
        
		write_imagef(output, globalId, res);
	}
}





