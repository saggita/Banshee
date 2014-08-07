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
//#define TRAVERSAL_STACKLESS
#define TRAVERSAL_STACKED
//#define LOCAL_STACK
#define NODE_STACK_SIZE 44
#define MAX_PATH_LENGTH 3

#define RAY_EPSILON 0.00001f
#define NUM_SAMPLES 1.f

#define MATERIAL1 0
#define MATERIAL2 1
#define MATERIAL3 2

#define BSDF_TYPE_LAMBERT  1
#define BSDF_TYPE_SPECULAR 2
#define BSDF_TYPE_EMISSIVE 3

#define NULL 0
#define DEFAULT_DEPTH 10000.f;

#define  M_PI 3.141592653589f
#define  AMBIENT_LIGHT 0.2f
//#define  AO_ENABLE
#define  AO_SAMPLES 1
#define  AO_RADIUS  0.1f
#define  AO_MIN     0.2f

#define NUM_SAMPLES_X 1
#define NUM_SAMPLES_Y 1

#define PATH_TYPE_DIRECT 0
#define PATH_TYPE_INDIRECT 1
#define PATH_TYPE_AO 2

/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/
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
    uint uNext;
    uint uNumPrims;
} BVHNode;

typedef struct _Config
{
    float4 vBackgroundColor;

    float3 vCameraDir;
    float3 vCameraRight;
    float3 vCameraUp;
    float3 vCameraPos;

    float fCameraNearZ;
    float fCameraPixelSize;

    uint uOutputWidth;
    uint uOutputHeight;

    uint uNumPointLights;
    uint uNumAreaLights;
    uint uNumRandomNumbers;
    uint uFrameCount;
    uint uTextureCount;

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
    uint        uMaterialIdx;
    int         iPixelRef;
    float       fDistance;
    bool        bHit;
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
    __global float* fRands;
    uint            uBufferSize;

} RandomBuffer;

typedef struct
{
    float4 vKe;
    float4 vKd;
    float4 vKs;
    float  fEs;
    uint eBsdf;
    uint uTd;
} MaterialRep;

typedef struct
{
    uint uVal;
} RNG;

typedef struct
{
    uint uWidth;
    uint uHeight;
    uint uPoolOffset;
} TextureDesc;

typedef struct {
    __global BVHNode*       sBVH;
    __global Vertex*        sVertices;
    __global uint4*         sIndices;
    __global Config*        sParams;
    __global PointLight*    sPointLights;
    __global MaterialRep*	sMaterials;
    __global TextureDesc*   sTextureDesc;
    __global float4*        vTextures;
    __global uint*          uAreaLights;
    __global uint*          uBVHIndices;
} SceneData;

typedef struct 
{
    Ray  sRay;
    int  iId;

    uint  uPathLength;
    uint  uPathOffset;
    uint  uLastMiss;
    uint  ePathType;
} PathStart;

typedef struct
{
    Ray  ray;
    uint data;
} path_extension_request;

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
    sRNG->uVal = WangHash(1664525U * sRNG->uVal + 1013904223U);
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

float4 make_float4(float x, float y, float z, float w)
{
    float4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}


float3 make_float3(float x, float y, float z)
{
    float3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

int2 make_int2(int x, int y)
{
    int2 res;
    res.x = x;
    res.y = y;
    return res;
}

float4 tex2d(SceneData* sSceneData, uint texIdx, float2 uv)
{
    uv.x -= floor(uv.x);
    uv.y -= floor(uv.y);

    TextureDesc desc = sSceneData->sTextureDesc[texIdx];

    __global float4* vTextureData = sSceneData->vTextures + desc.uPoolOffset;

    uint x = floor(uv.x * desc.uWidth - 0.5);
    uint y = floor(uv.y * desc.uHeight - 0.5);

    x = x % desc.uWidth;
    y = y % desc.uHeight;

    return vTextureData[y * desc.uWidth + x];
}

// Intersect Ray against triangle
bool IntersectTriangle(Ray* sRay, float3 vP1, float3 vP2, float3 vP3, float* fA, float* fB)
{
    float3 vE1 = sub(vP2, vP1);
    float3 vE2 = sub(vP3, vP1);

    float3 vS1 = cross(sRay->d, vE2);
    float  fInvDir = 1.f/dot(vS1, vE1);

    float3 vD = sRay->o - vP1;
    float  fB1 = dot(vD, vS1) * fInvDir;

    if (fB1 < 0.f || fB1 > 1.f)
        return false;

    float3 vS2 = cross(vD, vE1);
    float  fB2 = dot(sRay->d, vS2) * fInvDir;

    if (fB2 < 0.f || fB1 + fB2 > 1.f)
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
bool IntersectBox(Ray* sRay, float3 vRayDir, BBox sBox)
{
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

        float2 t1 = vertices[triangle.x].vTex;
        float2 t2 = vertices[triangle.y].vTex;
        float2 t3 = vertices[triangle.z].vTex;

        float a = 0;
        float b = 0;
        bool thisHit = IntersectTriangle(r, v1.xyz, v2.xyz, v3.xyz, &a, &b);

        if (thisHit)
        {
            shadingData->vPos = (1.f - a - b) * v1.xyz + a * v2.xyz + b * v3.xyz;
            shadingData->vNormal = normalize((1.f - a - b) * n1.xyz + a * n2.xyz + b * n3.xyz);
            shadingData->vTex = (1.f - a - b) * t1.xy + a * t2.xy + b * t3.xy;

            if (dot(-r->d, shadingData->vNormal) < 0)
                shadingData->vNormal = - shadingData->vNormal;

            shadingData->uMaterialIdx = triangle.w;
        }

        hit |= thisHit;
    }

    return hit;
}

// sample triangle
void SampleTriangleUniform(__global Vertex* sVertices, __global uint4* uIndices, uint uIdx, RNG* sRNG, float3* vPoint, float3* vNormal, float* fPDF)
{
    float a = RandFloat(sRNG);
    float b = RandFloat(sRNG);

    uint4 vTriangle = uIndices[uIdx];

    float4 v1 = sVertices[vTriangle.x].vPos;
    float4 v2 = sVertices[vTriangle.y].vPos;
    float4 v3 = sVertices[vTriangle.z].vPos;

    float4 n1 = sVertices[vTriangle.x].vNormal;
    float4 n2 = sVertices[vTriangle.y].vNormal;
    float4 n3 = sVertices[vTriangle.z].vNormal;

    *vPoint = (1.f - sqrt(a)) * v1.xyz + sqrt(a) * (1.f - b) * v2.xyz + sqrt(a) * b * v3.xyz;
    *vNormal = normalize((1.f - sqrt(a)) * n1.xyz + sqrt(a) * (1.f - b) * n2.xyz + sqrt(a) * b * n3.xyz);
    *fPDF = length(cross((v3-v1), (v3-v2))) * 0.5f;
};


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

    float3 vRayDir  = make_float3(1.0/r->d.x, 1.0/r->d.y, 1.0/r->d.z);

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
            bool radd = IntersectBox(r, vRayDir, rbox);
            bool ladd = IntersectBox(r, vRayDir, lbox);

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








// intersect Ray against the whole BVH structure
bool TraverseBVHStackless(
    __global BVHNode* nodes, 
    __global Vertex* vertices,
    __global uint4* indices, 
    Ray* r, 
    ShadingData* shadingData)
{
    bool hit = false;
    float3 vRayDir  = make_float3(1.0/r->d.x, 1.0/r->d.y, 1.0/r->d.z);

    uint idx = 0;
    while (idx != 0xFFFFFFFF)
    {
        uint  uNumPrims = nodes[idx].uNumPrims;

        // try intersecting against current node's bounding box
        // if this is the leaf try to intersect against contained triangle
        if (IntersectBox(r, vRayDir, nodes[idx].box))
        {
            if (uNumPrims != 0)
            {
                hit |= IntersectLeaf(vertices, indices, nodes[idx].uPrimStartIdx,  uNumPrims, r, shadingData);
                idx = nodes[idx].uNext;
                continue;
            }
            // traverse child nodes otherwise
            else
            {
                idx = idx + 1;
                continue;
            }
        }
        else
        {
                idx = nodes[idx].uNext;
        }
    };

    return hit;
}









float3 GetOrthoVector(float3 n)
{
    float3 p;
    if (fabs(n.z) > 0.707106781186547524401f) {
        float k = sqrt(n.y*n.y + n.z*n.z);
        p.x = 0; p.y = -n.z/k; p.z = n.y/k;
    }
    else {
        float k = sqrt(n.x*n.x + n.y*n.y);
        p.x = -n.y/k; p.y = n.x/k; p.z = 0;
    }

    return p;
}

float3 GetHemisphereSample(float3 vNormal, float e, RNG* sRNG)
{
    float3 vU = GetOrthoVector(vNormal);

    float3 vV = cross(vU, vNormal);
    vU = cross(vNormal, vV);

    float fR1 = RandFloat(sRNG);
    float fR2 = RandFloat(sRNG);

    float fSinPsi = sin(2*M_PI*fR1);
    float fCosPsi = cos(2*M_PI*fR1);
    float fCosTheta = pow(1.f - fR2, 1.f/(e + 1.f));
    float fSinTheta = sqrt(1.f - fCosTheta * fCosTheta);

    return vU * fSinTheta * fCosPsi + vV * fSinTheta * fSinPsi + vNormal * fCosTheta;
}

MaterialRep GetMaterial(uint uMaterialIdx, SceneData* sSceneData, RNG* sRNG)
{
    return sSceneData->sMaterials[uMaterialIdx];
}

bool IsEmissive(MaterialRep* sMaterial)
{
    return length(sMaterial->vKe) > 0.0;
}

int SampleMaterial(MaterialRep* sMaterialRep, ShadingData* sShadingData, float3 vWo, RNG* sRNG, Ray* sRay)
{
    switch (sMaterialRep->eBsdf)
    {
     case BSDF_TYPE_LAMBERT:
         {
            sRay->d = GetHemisphereSample(sShadingData->vNormal, 1.f, sRNG);
            sRay->o = sShadingData->vPos + RAY_EPSILON * sRay->d;
            sRay->maxt = 10000.f;
            sRay->mint = 0.f;
            return 1;
        }
    case BSDF_TYPE_EMISSIVE:
        {
            return false;
        }
    case BSDF_TYPE_SPECULAR:
        {
            if (dot( sShadingData->vNormal, vWo) > 0)
            {
                float3 vReflDir = normalize(-vWo + 2*dot(vWo, sShadingData->vNormal) * sShadingData->vNormal);

                sRay->o = sShadingData->vPos + RAY_EPSILON * vReflDir;


                sRay->d = normalize(GetHemisphereSample(vReflDir, sMaterialRep->fEs, sRNG));


                // As we generate the sample in the solid angle around reflection vector
                // there is a chance of generating a ray under the tangent plane
                // resulting in a darker image near reflective sphere silhouette for example
                // So we reflect the ray against the tangent plane in this case
                float fProj = dot(sShadingData->vNormal, sRay->d);
                if ( fProj < 0.f )
                {
                    sRay->d += 2 * sShadingData->vNormal * fabs(fProj);
                }

                sRay->mint = 0.f;
                sRay->maxt = 10000.f;
                return true;
            }
        }
    }
}


typedef struct
{
    __global TextureDesc*   sTextureDesc;
    __global float4*        vTextures;
} TextureSystem;

float4 tex2dsys(TextureSystem* sTextureSystem, uint texIdx, float2 uv)
{
    uv.x -= floor(uv.x);
    uv.y -= floor(uv.y);

    TextureDesc desc = sTextureSystem->sTextureDesc[texIdx];

    __global float4* vTextureData = sTextureSystem->vTextures + desc.uPoolOffset;

    uint x = floor(uv.x * desc.uWidth - 0.5);
    uint y = floor(uv.y * desc.uHeight - 0.5);

    x = x % desc.uWidth;
    y = y % desc.uHeight;

    return vTextureData[y * desc.uWidth + x];
}

float4 EvaluateMaterialSys(TextureSystem* sTextureSystem, MaterialRep* sMaterialRep, ShadingData* sShadingData, float3 vWi, float3 vWo, float4 vRadiance)
{
    switch (sMaterialRep->eBsdf)
    {
    case BSDF_TYPE_LAMBERT:
        {
            float  fInvPi = 1.f / M_PI;
            float4 vColor = (sMaterialRep->uTd == -1) ? sMaterialRep->vKd : tex2dsys(sTextureSystem, sMaterialRep->uTd, sShadingData->vTex);
            return fInvPi * vRadiance * vColor;
        }
    case BSDF_TYPE_SPECULAR:
        {
            float3 vReflDir = normalize(-vWi + 2.f*dot(vWi, sShadingData->vNormal) * sShadingData->vNormal);
            return vRadiance * sMaterialRep->vKs * pow(max(0.f, dot(vReflDir, vWo)), sMaterialRep->fEs) / (dot(sShadingData->vNormal, vWi));
        }
    case BSDF_TYPE_EMISSIVE:
        {
            return sMaterialRep->vKe;
        }
    }
}


__kernel void generate_rays(__global Config*                    gp_params,
                            __global path_extension_request*    gp_primary_rays
                            )
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);
    int flat_global_id = global_id.y * gp_params->uOutputWidth + global_id.x;

    unsigned width  = gp_params->uOutputWidth;
    unsigned height = gp_params->uOutputHeight;

    int  subsample = flat_global_id % (NUM_SAMPLES_X * NUM_SAMPLES_Y);
    int  pixel = flat_global_id / (NUM_SAMPLES_X * NUM_SAMPLES_Y);
    int2 subsample_idx = make_int2(subsample % NUM_SAMPLES_X, subsample / NUM_SAMPLES_X);
    int2 pixel_coords = make_int2(pixel % width, pixel / width);

    if (pixel_coords.x < width && pixel_coords.y < height)
    {
        // 1. Calculate pixel coordinates
        // 2. Calculate sample index
        // 3. Construct path start
        // 4. Add it into the buffer
        RNG rng = CreateRNG((get_global_id(0) + 133) * (get_global_id(1) + 177) * (gp_params->uFrameCount + 57));

        float pixel_size = gp_params->fCameraPixelSize;
        float near_z     = gp_params->fCameraNearZ;

        int x = pixel_coords.x - (width >> 1);
        int y = pixel_coords.y - (height >> 1);

        float fy = y + (subsample_idx.y + RandFloat(&rng))/NUM_SAMPLES_Y;
        float fx = x + (subsample_idx.x + RandFloat(&rng))/NUM_SAMPLES_X;

        Ray rr;
        rr.o = gp_params->vCameraPos;
        rr.d.x = gp_params->vCameraDir.x * near_z + gp_params->vCameraUp.x * fy * pixel_size + gp_params->vCameraRight.x * fx * pixel_size;
        rr.d.y = gp_params->vCameraDir.y * near_z  + gp_params->vCameraUp.y * fy * pixel_size+ gp_params->vCameraRight.y * fx * pixel_size;
        rr.d.z = gp_params->vCameraDir.z * near_z  + gp_params->vCameraUp.z * fy * pixel_size + gp_params->vCameraRight.z * fx * pixel_size;

        rr.d = normalize(rr.d);

        rr.mint = 0.f;
        rr.maxt = DEFAULT_DEPTH;

        gp_primary_rays[flat_global_id].ray = rr;
        gp_primary_rays[flat_global_id].data = flat_global_id;
    }
}

__kernel void process_extension_request(__global path_extension_request*      gp_requests,
                                        __global int*                         gp_active_request_indices,
                                          __global BVHNode*                   gp_bvh,
                                          __global Vertex*                    gp_vertices,
                                          __global uint4*                     gp_indices,
                                          __global PathVertex*                gp_intersections,
                                          __global int*                       gp_hit_results,
                                          uint                                gu_num_requests
                                          )
{

    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_requests)
    {

        Ray r = gp_requests[gp_active_request_indices[global_id]].ray;

        PathVertex  path_vertex;
        path_vertex.bHit = TraverseBVHStackless(gp_bvh, gp_vertices, gp_indices, &r, &path_vertex.sShadingData);
        gp_hit_results[gp_active_request_indices[global_id]] = (int)path_vertex.bHit;

        path_vertex.vIncidentDir = r.d;
        path_vertex.fDistance = r.maxt;
        gp_intersections[gp_active_request_indices[global_id]] = path_vertex;
    }
}


__kernel void sample_direct_illumination(__global BVHNode*      gp_bvh,
                                         __global Vertex*       gp_vertices,
                                         __global uint4*        gp_indices,
                                         __global PathVertex*   gp_intersections,
                                         __global int*          gp_active_ray_indices,
                                         __global MaterialRep*  gp_materials,
                                         __global TextureDesc*  gp_texture_descs,
                                         __global float4*       gp_texture_data,
                                         __global uint*         gp_area_light_indices,
                                         __global int*          gp_area_light_count,
                                         __global float4*       gp_radiance_values,
                                         uint                   gu_num_active_paths,
                                         uint                   gu_frame_count
                                         )
{

    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_active_paths)
    {
        TextureSystem texture_system;
        texture_system.sTextureDesc = gp_texture_descs;
        texture_system.vTextures = gp_texture_data;

        int  ray_index = gp_active_ray_indices[global_id];

        RNG rng = CreateRNG((get_global_id(0) + 133)*(gu_frame_count + 57));
        PathVertex my_vertex = gp_intersections[ray_index];

        float4 res = make_float4(0,0,0,0);

        uint i = (uint)(RandFloat(&rng) * (*gp_area_light_count + 1));
        if (i == *gp_area_light_count) --i;

        float3 light_sample, light_normal;
        float  pdf;

        SampleTriangleUniform(gp_vertices, gp_indices, gp_area_light_indices[i], &rng, &light_sample, &light_normal, &pdf);

        float3 light_dir = normalize(light_sample - my_vertex.sShadingData.vPos);
        float  dist = length(light_sample - my_vertex.sShadingData.vPos);

        Ray ray;
        ray.o = my_vertex.sShadingData.vPos + RAY_EPSILON * light_dir;
        ray.d = light_dir;
        ray.maxt = (1.f - 2 * RAY_EPSILON) * dist;
        ray.mint = 0.f;

        float n_dot_l = dot(my_vertex.sShadingData.vNormal, light_dir);
        float n_dot_wo = max(0.f, dot(light_normal, -light_dir));

        //Ray sRay;
        //sRay.d = GetHemisphereSample(myVertex.sShadingData.vNormal, 1.f, &sRNG);
        //sRay.o = myVertex.sShadingData.vPos + RAY_EPSILON * sRay.d;
        //sRay.maxt = 10000.f;
        //sRay.mint = 0.f;

        ShadingData temp_data;
        float shadow_factor = TraverseBVHStackless(gp_bvh, gp_vertices, gp_indices, &ray, &temp_data) ? 0.f : 1.f;

        MaterialRep material = gp_materials[my_vertex.sShadingData.uMaterialIdx];

        if (!IsEmissive(&material))
        {
            res = shadow_factor * n_dot_l * n_dot_wo * EvaluateMaterialSys(&texture_system, &material, &my_vertex.sShadingData, ray.d, -my_vertex.vIncidentDir, make_float4(100,100,75,100)) / (dist * dist * pdf);
        }
        else
        {
            res = material.vKe;
        }

        gp_radiance_values[ray_index] += res;
    }
}


__kernel void sample_direct_illumination_env(__global BVHNode*      gp_bvh,
                                         __global Vertex*       gp_vertices,
                                         __global uint4*        gp_indices,
                                         __global PathVertex*   gp_intersections,
                                         __global int*          gp_active_ray_indices,
                                         __global MaterialRep*  gp_materials,
                                         __global TextureDesc*  gp_texture_descs,
                                         __global float4*       gp_texture_data,
                                         __global uint*         gp_area_light_indices,
                                         __global int*          gp_area_light_count,
                                         __global float4*       gp_radiance_values,
                                         uint                   gu_num_active_paths,
                                         uint                   gu_frame_count
                                         )
{

    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_active_paths)
    {
        TextureSystem texture_system;
        texture_system.sTextureDesc = gp_texture_descs;
        texture_system.vTextures = gp_texture_data;

        int  ray_index = gp_active_ray_indices[global_id];

        RNG rng = CreateRNG((get_global_id(0) + 133)*(gu_frame_count + 57));
        PathVertex my_vertex = gp_intersections[ray_index];

        float4 res = make_float4(0,0,0,0);

        //uint i = (uint)(RandFloat(&rng) * (*gp_area_light_count + 1));
        //if (i == *gp_area_light_count) --i;

        //float3 light_sample, light_normal;
        //float  pdf;

        //SampleTriangleUniform(gp_vertices, gp_indices, gp_area_light_indices[i], &rng, &light_sample, &light_normal, &pdf);

        //float3 light_dir = normalize(light_sample - my_vertex.sShadingData.vPos);
        //float  dist = length(light_sample - my_vertex.sShadingData.vPos);

        //Ray ray;
        //ray.o = my_vertex.sShadingData.vPos + RAY_EPSILON * light_dir;
        //ray.d = light_dir;
        //ray.maxt = (1.f - 2 * RAY_EPSILON) * dist;
        //ray.mint = 0.f;

        Ray ray;
        ray.d = GetHemisphereSample(my_vertex.sShadingData.vNormal, 1.f, &rng);
        ray.o = my_vertex.sShadingData.vPos + RAY_EPSILON * ray.d;
        ray.maxt = 10000.f;
        ray.mint = 0.f;


        float n_dot_l = dot(my_vertex.sShadingData.vNormal, ray.d);

        ShadingData temp_data;
        float shadow_factor = TraverseBVHStackless(gp_bvh, gp_vertices, gp_indices, &ray, &temp_data) ? 0.f : 1.f;

        MaterialRep material = gp_materials[my_vertex.sShadingData.uMaterialIdx];

        res = shadow_factor * n_dot_l * EvaluateMaterialSys(&texture_system, &material, &my_vertex.sShadingData, ray.d, -my_vertex.vIncidentDir, make_float4(1600,1400,1450,1000)) / M_PI;

        gp_radiance_values[ray_index] += res;
    }
}

__kernel void export_radiance(__global Config*                    gp_params,
                               __global float4*    gp_radiance_values,
                               __write_only image2d_t gt_output_image,
                               uint gu_num_values)
{
    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_values)
    {
        int2 pixel_coord = make_int2(global_id % gp_params->uOutputWidth, global_id / gp_params->uOutputWidth);
        write_imagef(gt_output_image, pixel_coord, gp_radiance_values[global_id]);
    }
}

__kernel void sample_material(
                              __global PathVertex*   gp_intersections,
                              __global int*          gp_active_ray_indices,
                              __global MaterialRep*  gp_materials,
                              __global TextureDesc*  gp_texture_descs,
                              __global float4*       gp_texture_data,
                              uint                   gu_num_active_paths,
                              uint                   gu_frame_count,
                              __global path_extension_request* gp_requests,
                              __global int*          gp_sample_results
                              )
{
    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_active_paths)
    {
        TextureSystem texture_system;
        texture_system.sTextureDesc = gp_texture_descs;
        texture_system.vTextures = gp_texture_data;

        int  ray_index = gp_active_ray_indices[global_id];

        RNG rng = CreateRNG((get_global_id(0) + 133)*(gu_frame_count + 57));
        PathVertex my_vertex = gp_intersections[ray_index];

        MaterialRep material = gp_materials[my_vertex.sShadingData.uMaterialIdx];

        Ray r;
        bool result = SampleMaterial(&material, &my_vertex.sShadingData, -my_vertex.vIncidentDir, &rng, &r);
        gp_sample_results[ray_index] = result ? 1 : 0;

        if (result)
        {
            gp_requests[ray_index].ray = r;
        }
    }
}


__kernel void evaluate_material(
                               __global PathVertex*   gp_intersections,
                               __global PathVertex*   gp_intersections_prev,
                               __global int*          gp_active_ray_indices,
                               __global MaterialRep*  gp_materials,
                               __global TextureDesc*  gp_texture_descs,
                               __global float4*       gp_texture_data,
                               __global float4*       gp_in_radiance_values,
                               __global float4*       gp_out_radiance_values,
                               uint                   gu_num_active_paths,
                               uint                   gu_frame_count
                               )
{
    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_active_paths)
    {
        TextureSystem texture_system;
        texture_system.sTextureDesc = gp_texture_descs;
        texture_system.vTextures = gp_texture_data;

        int  ray_index = gp_active_ray_indices[global_id];

        RNG rng = CreateRNG((get_global_id(0) + 133)*(gu_frame_count + 57));
        PathVertex my_vertex = gp_intersections[ray_index];
        PathVertex prev_vertex = gp_intersections_prev[ray_index];
        MaterialRep material = gp_materials[my_vertex.sShadingData.uMaterialIdx];

        float n_dot_l = dot( my_vertex.sShadingData.vNormal, prev_vertex.vIncidentDir );
        float dist = length(my_vertex.sShadingData.vPos - prev_vertex.sShadingData.vPos);
        float4 res = n_dot_l * EvaluateMaterialSys(&texture_system, &material, &my_vertex.sShadingData, prev_vertex.vIncidentDir, -my_vertex.vIncidentDir, gp_in_radiance_values[ray_index]);
        gp_out_radiance_values[ray_index] += res;
    }
}

__kernel void resolve_radiance(
                               __global float4*    gp_in_radiance_values,
                               __global float4*    gp_out_radiance_values,
                               uint gu_num_values,
                               uint gu_frame_count
                               )
{
    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_values)
    {
        float4 val = gp_in_radiance_values[global_id] / (gu_frame_count + 1);
        gp_out_radiance_values[global_id] = val;
    }
}

__kernel void tonemap_radiance(
                               __global float4*    gp_in_radiance_values,
                               __global float4*    gp_out_radiance_values,
                               uint  gu_num_values,
                               float gf_avg_loglum,
                               float gf_time_delta
                               )
{
    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_values)
    {
        float4 val = gp_in_radiance_values[global_id];
        float mid_gray = 0.6f;

        val.xyz *= mid_gray/(exp(gf_avg_loglum) + 0.001f);
        val.x /= (1.0f + val.x);
        val.y /= (1.0f + val.y);
        val.z /= (1.0f + val.z);

        gp_out_radiance_values[global_id] = val;
    }
}

__kernel void calculate_loglum(
                               __global float4*    gp_in_radiance_values,
                               __global float*     gp_out_loglum_values,
                               uint gu_num_values
                               )
{
    int global_id  = get_global_id(0);
    int local_id   = get_local_id(0);
    int group_size = get_local_size(0);

    if (global_id < gu_num_values)
    {
        float3 LUM = make_float3(0.2125f, 0.7154f, 0.0721f); 
        float4 val = gp_in_radiance_values[global_id];
        gp_out_loglum_values[global_id] = clamp(log(dot(val.xyz, LUM) + 0.001f), 0.f, 10000.f);
    }
}

__kernel void reorder_bvh_indices(__global uint4* gp_in_indices,
                                  __global uint*  gp_remap_indices,
                                  __global MaterialRep*  gp_materials,
                                           uint   gu_num_indices,
                                  __global uint4* gp_out_indices,
                                  __global uint*  gp_area_lights,
                                  __global int*   gp_area_light_count
                                  )
{
    int global_id  = get_global_id(0);
    
    if (global_id < gu_num_indices)
    {
        uint4 val = gp_in_indices[gp_remap_indices[global_id]];
        gp_out_indices[global_id] = val;
        
        if (gp_materials[val.w].eBsdf == BSDF_TYPE_EMISSIVE)
        {
            int idx = atomic_add(gp_area_light_count, 1);
            gp_area_lights[idx] = global_id;
        }
    }
}






