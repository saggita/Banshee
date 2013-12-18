/// Extensions
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable


//#define TRAVERSAL_STACKLESS
#define TRAVERSAL_STACKED
//#define TRAVERSAL_PACKET
//#define LOCAL_STACK
#define NODE_STACK_SIZE 32
#define NODE_SHARED_STACK_SIZE 128
#define TILE_SIZE_X 8
#define TILE_SIZE_Y 8


//#define atom_xchg(x,y) 0
//#define atom_max(x,y) 0
//#define atom_min(x,y) 0
//#define atom_inc(x) 0

/// Type definitions
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

} Config;

typedef struct _Ray
{
	float3 o;
	float3 d;
	float  mint;
	float  maxt;
} Ray;

typedef  struct {
	uint  uCount;
	uint  uInstanceCount;
	uint  uFirstIndex;
	uint  uBaseVertex;
	uint  uBaseInstance;
} DrawElementsIndirectCommand;

/// Macro definitions
#define NULL 0
#define DEFAULT_DEPTH 10000.f;
//#define TRACE_INTERNAL_ONLY


/// Helper functions

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

float2 make_float2(float x, float y)
{
	float2 res;
	res.x = x;
	res.y = y;
	return res;
}

int2 make_int2(int x, int y)
{
	int2 res;
	res.x = x;
	res.y = y;
	return res;
}

// substract float3 vectors
float3 sub(float3 v1, float3 v2)
{
	float3 res;
	res.x = v1.x - v2.x;
	res.y = v1.y - v2.y;
	res.z = v1.z - v2.z;
	return res;
}


float ComponentMask(float3 v, int comp)
{
	float3 component_mask_3[3] = 
	{
		{1.0, 0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 0.0, 1.0}
	};

	return dot(v, component_mask_3[comp]);
}

float4 TransformPoint(float16 mvp, float4 p)
{
	float4 res;
	res.w = mvp.sc * p.x + mvp.sd * p.y + mvp.se * p.z + mvp.sf * p.w;
	res.x = mvp.s0 * p.x + mvp.s1 * p.y + mvp.s2 * p.z + mvp.s3 * p.w;
	res.y = mvp.s4 * p.x + mvp.s5 * p.y + mvp.s6 * p.z + mvp.s7 * p.w;
	res.z = mvp.s8 * p.x + mvp.s9 * p.y + mvp.sa * p.z + mvp.sb * p.w;
	return res;
}

bool FrustumCheck(float4 p)
{
	float invw = 1.0 / p.w;
	return p.x * invw >= -1 && p.x * invw <= 1 && p.y * invw >= -1 && p.y * invw <= 1 &&
		p.z * invw >= -1 && p.z * invw <= 1;
}


float4 FindMinZ(float4 v1, float4 v2)
{
	return (v1.z < v2.z) ? (v1) : (v2);
}


// intersect Ray against triangle
bool IntersectTriangle(Ray* r, float3 v1, float3 v2, float3 v3)
{
	float3 e1 = sub(v2, v1);
	float3 e2 = sub(v3, v1);

	float3 s1 = cross(r->d, e2);
	float invDir = 1.0/dot(s1, e1);

	//if (div == 0.f)
	//return false;

	float3 d = r->o - v1;
	float b1 = dot(d,s1) * invDir;

	if (b1 < 0 || b1 > 1)
		return false;

	float3 s2 = cross(d, e1);
	float b2 = dot(r->d, s2) * invDir;

	if (b2 < 0 || b1 + b2 > 1)
		return false;

	float temp = dot(e2, s2) * invDir;

	if (temp > 0 && temp <= r->maxt)
	{
		r->maxt = temp;
		return true;
	}

	return false;
}

// intersect Ray with the axis-aligned box
bool IntersectBox(Ray* r, BBox box)
{
	float3 rayDir  = make_float3(1.0/r->d.x, 1.0/r->d.y, 1.0/r->d.z);
	float lo = rayDir.x*(box.vMin.x - r->o.x);
	float hi = rayDir.x*(box.vMax.x - r->o.x);

	float tmin = fmin(lo, hi);
	float tmax = fmax(lo, hi);

	float lo1 = rayDir.y*(box.vMin.y - r->o.y);
	float hi1 = rayDir.y*(box.vMax.y - r->o.y);

	tmin = fmax(tmin, fmin(lo1, hi1));
	tmax = fmin(tmax, fmax(lo1, hi1));

	float lo2 = rayDir.z*(box.vMin.z - r->o.z);
	float hi2 = rayDir.z*(box.vMax.z - r->o.z);

	tmin = fmax(tmin, fmin(lo2, hi2));
	tmax = fmin(tmax, fmax(lo2, hi2));

	bool hit = (tmin <= tmax) && (tmax > 0.0);
	float t = tmin >=0 ? tmin : tmax;

	if (hit && tmin < r->maxt)
	{
		return true;
	}

	return false;
}

/// BVH related functions
//  intersect a Ray with BVH leaf
bool IntersectLeaf(__global float4* vertices, __global uint4* indices, uint idx, int uNumPrims, Ray* r)
{
	bool hit = false;
	for (int i = 0; i < uNumPrims; ++i)
	{
		uint4 triangle = indices[idx + i];

		float4 v1 = vertices[triangle.x];
		float4 v2 = vertices[triangle.y];
		float4 v3 = vertices[triangle.z];

		hit |= IntersectTriangle(r, v1.xyz, v2.xyz, v3.xyz);
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
	__global BVHNode* nodes, __global float4* vertices, __global uint4* indices, Ray* r)
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
		uint  left  = idx + 1;
		uint  uRight = nodes[idx].uRight;
		uint uPrimStartIdx = nodes[idx].uPrimStartIdx;
		uint  uNumPrims = nodes[idx].uNumPrims;

		// try intersecting against current node's bounding box
		// if this is the leaf try to intersect against contained triangle
		if (uNumPrims != 0)
		{
			hit |= IntersectLeaf(vertices, indices, uPrimStartIdx, uNumPrims, r);
		}
		// traverse child nodes otherwise
		else
		{
			bool radd = IntersectBox(r, nodes[uRight].box);
			bool ladd = IntersectBox(r, nodes[left].box);

			if (radd && ladd)
			{
				int axis = uPrimStartIdx;
				if (ComponentMask(r->d, axis) > 0)
				{
					NODE_STACK_PUSH(uRight);
					idx = left;
				} 
				else
				{
					NODE_STACK_PUSH(left);
					idx = uRight;
				}
			}
			else if (radd)
			{
				idx = uRight;
			}
			else if (ladd)
			{
				idx = left;
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

bool anythreads(__local int* flag, int val)
{
	if (get_local_id(0) == 0 && get_local_id(1) == 0)
		*flag = 0;
	barrier(CLK_LOCAL_MEM_FENCE);
	atom_max(flag, val);
	barrier(CLK_LOCAL_MEM_FENCE);
	return *flag > 0;
}

// intersect Ray against the whole BVH structure
bool TraverseBVHPacket(
#ifdef LOCAL_STACK
	__local int* stack, 
#endif
	__local int* flag,
	__global BVHNode* nodes, __global float4* vertices, __global uint4* indices, Ray* r)
{
	// init node stack
	NODE_STACK_INIT(NODE_SHARED_STACK_SIZE);

	bool hit = false;

	// start from the root
	uint idx = 0;
	do
	{
		// load current node
		BBox box = nodes[idx].box;
		uint  left  = idx + 1;
		uint  uRight = nodes[idx].uRight;
		uint uPrimStartIdx = nodes[idx].uPrimStartIdx;
		uint  uNumPrims = nodes[idx].uNumPrims;
		int radd = 0;
		int ladd = 0;

		// try intersecting against current node's bounding box
		// if this is the leaf try to intersect against contained triangle
		if (uNumPrims != 0)
		{
			hit |= IntersectLeaf(vertices, indices, uPrimStartIdx, uNumPrims, r);
		}
		// traverse child nodes otherwise
		else
		{
			radd = IntersectBox(r, nodes[uRight].box) ? 1 : 0;
			ladd = IntersectBox(r, nodes[left].box) ? 1 : 0;
		}

		bool rany = anythreads(flag, radd);
		bool lany = anythreads(flag, ladd);

		if (get_local_id(0) == 0 && get_local_id(1) == 0)
		{
			if (lany) NODE_STACK_PUSH(left);
			if (rany) NODE_STACK_PUSH(uRight);
			*flag = NODE_STACK_POP;
		}

		barrier(CLK_LOCAL_MEM_FENCE);

		// pop next item from the stack
		idx = *flag;
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

bool TraverseBVHStackless(__global BVHNode* nodes, __global float4* vertices, __global uint4* indices, Ray* r)
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
					hit |= IntersectLeaf(vertices, indices, nodes[current].uPrimStartIdx, nodes[current].uNumPrims, r);
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
					hit |= IntersectLeaf(vertices, indices, nodes[current].uPrimStartIdx, nodes[current].uNumPrims, r);
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


/// Raytrace depth kernel
__kernel void TraceDepth(
	__global BVHNode* bvh,
	__global float4* vertices,
	__global uint4* indices,
	__global Config* params,
	__write_only image2d_t output
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

	int2 localId;
	localId.x = get_local_id(0);
	localId.y = get_local_id(1);

	int flatLocalId = localId.y * get_local_size(0) + localId.x;

	if (globalId.x < params->uOutputWidth && globalId.y < params->uOutputHeight)
	{
		unsigned width = params->uOutputWidth;
		unsigned height = params->uOutputHeight;
		float pixel_size = params->fCameraPixelSize;
		float near_z = params->fCameraNearZ;

		int xc = globalId.x - (width >> 1);
		int yc = globalId.y - (height >> 1);

		Ray rr;
		rr.o = params->vCameraPos;
		rr.d.x = params->vCameraDir.x * near_z + params->vCameraUp.x * (yc + 0.5f) * pixel_size + params->vCameraRight.x * (xc + 0.5f) * pixel_size;
		rr.d.y = params->vCameraDir.y * near_z + params->vCameraUp.y * (yc + 0.5f) * pixel_size + params->vCameraRight.y * (xc + 0.5f) * pixel_size;
		rr.d.z = params->vCameraDir.z * near_z + params->vCameraUp.z * (yc + 0.5f) * pixel_size + params->vCameraRight.z * (xc + 0.5f) * pixel_size;

		rr.d = normalize(rr.d);

		rr.mint = 0.f;
		rr.maxt = DEFAULT_DEPTH;

		float res = DEFAULT_DEPTH;

#if defined(TRAVERSAL_STACKLESS)
		bool hit  = TraverseBVHStackless(bvh, vertices, indices, &rr);
#elif defined(TRAVERSAL_STACKED)
#if defined(LOCAL_STACK)
		__local int* this_thread_stack = stack + NODE_STACK_SIZE * flatLocalId;
		bool hit  = TraverseBVHStacked(this_thread_stack, bvh, vertices, indices, &rr);
#else
		bool hit  = TraverseBVHStacked(bvh, vertices, indices, &rr);
#endif
#elif defined(TRAVERSAL_PACKET)
		bool hit  = TraverseBVHPacket(stack, &flag, bvh, vertices, indices, &rr);
#endif
		if (hit)
		{
			res = rr.maxt;
		}

		write_imagef(output, globalId, res);
	}
}


float4 TransformPointHdiv( float16 pi, float4 p )
{
	p = TransformPoint(pi, p);
	p /= p.w;
	return p;
}

bool TestBSphere(__global Config* cfg, 
				 float4 bsphere,
				 float2 minmax)
{
	int gx = get_group_id(0);
	int gy = get_group_id(1);

	int tx = get_local_size(0);
	int ty = get_local_size(1);

	int nx = get_global_size(0) / tx;
	int ny = get_global_size(1) / ty;


	float tsx = 2.0 / nx;
	float tsy = 2.0 / ny;

	float l = (tsx * gx) - 1.0;
	float r = l + tsx;
	float b = (tsy * gy) - 1.0;
	float t = b + tsy;

	float4 v0 = TransformPointHdiv( cfg->mProjInv, make_float4( l, b, 0.1f, 1.f ) );
	float4 v1 = TransformPointHdiv( cfg->mProjInv, make_float4( r, b, 0.1f, 1.f ) );
	float4 v2 = TransformPointHdiv( cfg->mProjInv, make_float4( r, t, 0.1f, 1.f ) );
	float4 v3 = TransformPointHdiv( cfg->mProjInv, make_float4( l, t, 0.1f, 1.f ) );

	float4 eq0;
	eq0.xyz = normalize(cross(v0.xyz, v1.xyz)); eq0.w = 0;

	float4 eq1;
	eq1.xyz = normalize(cross(v2.xyz, v3.xyz)); eq1.w = 0;

	float4 eq2;
	eq2.xyz = normalize(cross(v1.xyz, v2.xyz)); eq2.w = 0;

	float4 eq3;
	eq3.xyz = normalize(cross(v3.xyz, v0.xyz)); eq3.w = 0;

	float zHalf = 0.5 * (minmax.x + minmax.y);
	float4 center = bsphere; center.w = 1;
	center = TransformPointHdiv(cfg->mView, center); 

	if( dot(eq0.xyz, center.xyz) >= -bsphere.w &&
		dot(eq1.xyz, center.xyz) >= -bsphere.w &&
		dot(eq2.xyz, center.xyz) >= -bsphere.w &&
		dot(eq3.xyz, center.xyz) >= -bsphere.w )
	{
		if( center.z - bsphere.w <= minmax.y && center.z >= -bsphere.w )
		{
			return true;
		}
	}

	return false;
}

__kernel void CheckVisibility(
	__global Config* cfg,
	uint numBounds,
	__global float4* bounds,
	__read_only image2d_t depthmap,
	__global uint* visiblity
	)
{
	__local uint lMin;
	__local uint lMax;

	int2 localId = make_int2(get_local_id(0), get_local_id(1));
	int2 groupId = make_int2(get_group_id(0), get_group_id(1));
	int2 groupDim = make_int2(get_local_size(0), get_local_size(1));

	int flatLocalId = localId.y * groupDim.x + localId.x;

	if (localId.x == 0 && localId.y == 0)
	{
		lMin = 10000.0;
		lMax = 0.0;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	int2 texCoord = make_int2(groupId.x * TILE_SIZE_X + localId.x, groupId.y * TILE_SIZE_Y + localId.y);

	if (texCoord.x < get_image_width(depthmap) && texCoord.y < get_image_height(depthmap))
	{
		float depth = read_imagef(depthmap, texCoord).x;
		atom_min(&lMin, as_uint(depth));
		atom_max(&lMax, as_uint(depth));
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = flatLocalId; i < numBounds; i += groupDim.x * groupDim.y)
	{
		int2 num_tiles = make_int2(cfg->uOutputWidth / TILE_SIZE_X, cfg->uOutputHeight / TILE_SIZE_Y); 

		if (TestBSphere(cfg, bounds[i], make_float2(as_float(lMin), as_float(lMax))))
		{
			atom_or(&visiblity[i], 1);
		}
	}
}

/// replace with scan further
__kernel void BuildCmdList( uint numBounds, 
						   __global Offset* offsets,  
						   __global DrawElementsIndirectCommand* commands, 
						   __global int* counter,
						   __global uint* visibility)
{
	uint globalId = get_global_id(0);

	if (globalId < numBounds)
	{
		if (visibility[globalId])
		{
			int idx = atom_inc(counter);

			__global DrawElementsIndirectCommand* cmd = commands + idx;

			cmd->uCount = offsets[globalId].uNumIndices;
			cmd->uInstanceCount = 1;
			cmd->uFirstIndex= offsets[globalId].uStartIdx;
			cmd->uBaseVertex = 0;
			cmd->uBaseInstance = 0;
		}
	}
}


