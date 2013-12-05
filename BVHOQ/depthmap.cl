/// Extensions
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable
 

//#define TRAVERSAL_STACKLESS
#define TRAVERSAL_STACKED
//#define TRAVERSAL_PACKET
#define LOCAL_STACK
#define NODE_STACK_SIZE 16
#define NODE_SHARED_STACK_SIZE 128

/// Type definitions
typedef struct _bbox
{
	float3 pmin;
	float3 pmax;
} bbox;

typedef struct _offset
{
	uint start_idx;
	uint num_idx;
} offset;

typedef struct _bvh_node
{
	bbox box;
	// prim_start_index == 0xffffffff for internal nodes
	uint prim_start_index;
	uint right;
	uint parent;
	uint num_prims;
} bvh_node;

typedef struct _config
{
	float16 mProjInv;
	float16 mView;

	float3 camera_dir;
	float3 camera_right;
	float3 camera_up;
	float3 camera_pos;

	float camera_near_z;
	float camera_pixel_size;

	unsigned output_width;
	unsigned output_height;

} config;

typedef struct _ray
{
	float3 o;
	float3 d;
	float  mint;
	float  maxt;
} ray;

typedef  struct {
	uint  count;
	uint  instanceCount;
	uint  firstIndex;
	uint  baseVertex;
	uint  baseInstance;
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


float component(float3 v, int comp)
{
	float3 component_mask_3[3] = 
	{
		{1.0, 0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 0.0, 1.0}
	};

	return dot(v, component_mask_3[comp]);
}

float4 transform_point(float16 mvp, float4 p)
{
	float4 res;
	res.w = mvp.sc * p.x + mvp.sd * p.y + mvp.se * p.z + mvp.sf * p.w;
	res.x = mvp.s0 * p.x + mvp.s1 * p.y + mvp.s2 * p.z + mvp.s3 * p.w;
	res.y = mvp.s4 * p.x + mvp.s5 * p.y + mvp.s6 * p.z + mvp.s7 * p.w;
	res.z = mvp.s8 * p.x + mvp.s9 * p.y + mvp.sa * p.z + mvp.sb * p.w;
	return res;
}

bool frustum_check(float4 p)
{
	float invw = 1.0 / p.w;
	return p.x * invw >= -1 && p.x * invw <= 1 && p.y * invw >= -1 && p.y * invw <= 1 &&
		p.z * invw >= -1 && p.z * invw <= 1;
}


float4 find_min_z(float4 v1, float4 v2)
{
	return (v1.z < v2.z) ? (v1) : (v2);
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

// intersect ray against triangle
bool intersect_triangle(ray* r, float3 v1, float3 v2, float3 v3)
{
	float3 e1 = sub(v2, v1);
	float3 e2 = sub(v3, v1);

	float3 s1 = cross(r->d, e2);
	float inv_div = 1.0/dot(s1, e1);

	//if (div == 0.f)
	//return false;

	float3 d = r->o - v1;
	float b1 = dot(d,s1) * inv_div;

	if (b1 < 0 || b1 > 1)
		return false;

	float3 s2 = cross(d, e1);
	float b2 = dot(r->d, s2) * inv_div;

	if (b2 < 0 || b1 + b2 > 1)
		return false;

	float temp = dot(e2, s2) * inv_div;

	if (temp > 0 && temp <= r->maxt)
	{
		r->maxt = temp;
		return true;
	}

	return false;
}

// intersect ray with the axis-aligned box
bool intersect_box(ray* r, bbox box)
{
	float3 ray_dir  = make_float3(1.0/r->d.x, 1.0/r->d.y, 1.0/r->d.z);
	float lo = ray_dir.x*(box.pmin.x - r->o.x);
	float hi = ray_dir.x*(box.pmax.x - r->o.x);

	float tmin = fmin(lo, hi);
	float tmax = fmax(lo, hi);

	float lo1 = ray_dir.y*(box.pmin.y - r->o.y);
	float hi1 = ray_dir.y*(box.pmax.y - r->o.y);

	tmin = fmax(tmin, fmin(lo1, hi1));
	tmax = fmin(tmax, fmax(lo1, hi1));

	float lo2 = ray_dir.z*(box.pmin.z - r->o.z);
	float hi2 = ray_dir.z*(box.pmax.z - r->o.z);

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
//  intersect a ray with BVH leaf
bool intersect_leaf(__global float4* vertices, __global uint4* indices, uint idx, int num_prims, ray* r)
{
	bool hit = false;
	for (int i = 0; i < num_prims; ++i)
	{
		uint4 triangle = indices[idx + i];

		float4 v1 = vertices[triangle.x];
		float4 v2 = vertices[triangle.y];
		float4 v3 = vertices[triangle.z];

		hit |= intersect_triangle(r, v1.xyz, v2.xyz, v3.xyz);
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


// intersect ray against the whole BVH structure
bool traverse_bvh_stacked(
#ifdef LOCAL_STACK
	__local int* stack, 
#endif
	__global bvh_node* nodes, __global float4* vertices, __global uint4* indices, ray* r)
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
		bbox box = nodes[idx].box;
		uint  left  = idx + 1;
		uint  right = nodes[idx].right;
		uint prim_start_index = nodes[idx].prim_start_index;
		uint  num_prims = nodes[idx].num_prims;

		// try intersecting against current node's bounding box
		// if this is the leaf try to intersect against contained triangle
		if (num_prims != 0)
		{
			hit |= intersect_leaf(vertices, indices, prim_start_index, num_prims, r);
		}
		// traverse child nodes otherwise
		else
		{
			bool radd = intersect_box(r, nodes[right].box);
			bool ladd = intersect_box(r, nodes[left].box);

			if (radd && ladd)
			{
				int axis = prim_start_index;
				if (component(r->d, axis) > 0)
				{
					NODE_STACK_PUSH(right);
					idx = left;
				} 
				else
				{
					NODE_STACK_PUSH(left);
					idx = right;
				}
			}
			else if (radd)
			{
				idx = right;
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

// intersect ray against the whole BVH structure
bool traverse_bvh_packet(
#ifdef LOCAL_STACK
	__local int* stack, 
#endif
	__local int* flag,
	__global bvh_node* nodes, __global float4* vertices, __global uint4* indices, ray* r)
{
	// init node stack
	NODE_STACK_INIT(NODE_SHARED_STACK_SIZE);

	bool hit = false;

	// start from the root
	uint idx = 0;
	do
	{
		// load current node
		bbox box = nodes[idx].box;
		uint  left  = idx + 1;
		uint  right = nodes[idx].right;
		uint prim_start_index = nodes[idx].prim_start_index;
		uint  num_prims = nodes[idx].num_prims;
		int radd = 0;
		int ladd = 0;

		// try intersecting against current node's bounding box
		// if this is the leaf try to intersect against contained triangle
		if (num_prims != 0)
		{
			hit |= intersect_leaf(vertices, indices, prim_start_index, num_prims, r);
		}
		// traverse child nodes otherwise
		else
		{
			radd = intersect_box(r, nodes[right].box) ? 1 : 0;
			ladd = intersect_box(r, nodes[left].box) ? 1 : 0;
		}

		bool rany = anythreads(flag, radd);
		bool lany = anythreads(flag, ladd);

		if (get_local_id(0) == 0 && get_local_id(1) == 0)
		{
			if (lany) NODE_STACK_PUSH(left);
			if (rany) NODE_STACK_PUSH(right);
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

#define PARENT(x) (nodes[(x)].parent)
#define LEFT_CHILD(x) ((x)+1)
#define RIGHT_CHILD(x) (nodes[(x)].right)

#define IS_LEAF(x) (nodes[(x)].num_prims != 0)

bool traverse_bvh_stackless(__global bvh_node* nodes, __global float4* vertices, __global uint4* indices, ray* r)
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

				else if (current == LEFT_CHILD(PARENT(current)))
				{
					current = RIGHT_CHILD(PARENT(current));
					state = FROM_SIBLING;
				}
				else
				{
					current = PARENT(current);
					state = FROM_CHILD;
				}
				break;
			}

		case FROM_SIBLING:
			{
				bbox box = nodes[current].box;
				if (!intersect_box(r, box))
				{
					current = PARENT(current);
					state = FROM_CHILD;
				}
				else if (IS_LEAF(current))
				{
					hit |= intersect_leaf(vertices, indices, nodes[current].prim_start_index, nodes[current].num_prims, r);
					current = PARENT(current);
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
				bbox box = nodes[current].box;
				if (!intersect_box(r, box))
				{
					current = RIGHT_CHILD(PARENT(current));
					state = FROM_SIBLING;
				}
				else if (IS_LEAF(current))
				{
					hit |= intersect_leaf(vertices, indices, nodes[current].prim_start_index, nodes[current].num_prims, r);
					current = RIGHT_CHILD(PARENT(current));
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
__kernel void trace_primary_depth(
				__global bvh_node* bvh,
				__global float4* vertices,
				__global uint4* indices,
				__global config* params,
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

	int2 global_id;
	global_id.x = get_global_id(0);
	global_id.y = get_global_id(1);

	int2 local_id;
	local_id.x = get_local_id(0);
	local_id.y = get_local_id(1);

	int flat_local_id = local_id.y * get_local_size(0) + local_id.x;

	if (global_id.x < params->output_width && global_id.y < params->output_height)
	{
		unsigned width = params->output_width;
		unsigned height = params->output_height;
		float pixel_size = params->camera_pixel_size;
		float near_z = params->camera_near_z;

		int xc = global_id.x - (width >> 1);
		int yc = global_id.y - (height >> 1);

		ray rr;
		rr.o = params->camera_pos;
		rr.d.x = params->camera_dir.x * near_z + params->camera_up.x * (yc + 0.5f) * pixel_size + params->camera_right.x * (xc + 0.5f) * pixel_size;
		rr.d.y = params->camera_dir.y * near_z + params->camera_up.y * (yc + 0.5f) * pixel_size + params->camera_right.y * (xc + 0.5f) * pixel_size;
		rr.d.z = params->camera_dir.z * near_z + params->camera_up.z * (yc + 0.5f) * pixel_size + params->camera_right.z * (xc + 0.5f) * pixel_size;

		rr.d = normalize(rr.d);

		rr.mint = 0.f;
		rr.maxt = DEFAULT_DEPTH;

		float res = DEFAULT_DEPTH;

#if defined(TRAVERSAL_STACKLESS)
		bool hit  = traverse_bvh_stackless(bvh, vertices, indices, &rr);
#elif defined(TRAVERSAL_STACKED)
	#if defined(LOCAL_STACK)
		__local int* this_thread_stack = stack + NODE_STACK_SIZE * flat_local_id;
		bool hit  = traverse_bvh_stacked(this_thread_stack, bvh, vertices, indices, &rr);
	#else
		bool hit  = traverse_bvh_stacked(bvh, vertices, indices, &rr);
	#endif
#elif defined(TRAVERSAL_PACKET)
		bool hit  = traverse_bvh_packet(stack, &flag, bvh, vertices, indices, &rr);
#endif
		if (hit)
		{
			res = rr.maxt;
		}

		write_imagef(output, global_id, res);
	}
}




#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16

float4 create_plane( float4 b, float4 c )
{
    float4 n;

    // normalize(cross( b.xyz-a.xyz, c.xyz-a.xyz )), except we know "a" is the origin
    n.xyz = normalize(cross( b.xyz, c.xyz ));

    // -(n dot a), except we know "a" is the origin
    n.w = 0;

    return n;
}

// point-plane distance, simplified for the case where 
// the plane passes through the origin
float sdist( float3 p, float4 eqn )
{
    // dot( eqn.xyz, p.xyz ) + eqn.w, , except we know eqn.w is zero 
    // (see CreatePlaneEquation above)
    return dot( eqn.xyz, p.xyz );
}

float4 transform_point_hdiv( float16 pi, float4 p )
{
    p = transform_point(pi, p);
    p /= p.w;
    return p;
}

bool test_bsphere(__global config* cfg, 
				  float4 bsphere, 
				  int2 tile_idx, 
				  int2 tile_dim, 
				  int2 num_tiles, 
				  float2 minmax)
{
	float4 frustumEqn[4];
    {   // construct frustum for this tile
        uint pxm = tile_dim.x*tile_idx.x;
        uint pym = tile_dim.y*tile_idx.y;
        uint pxp = tile_dim.x*(tile_idx.x+1);
        uint pyp = tile_dim.y*(tile_idx.y+1);

		uint uWindowWidthEvenlyDivisibleByTileRes = tile_dim.x*num_tiles.x;
        uint uWindowHeightEvenlyDivisibleByTileRes = tile_dim.y*num_tiles.y;

        // four corners of the tile, clockwise from top-left
        float4 frustum[4];
        /*frustum[0] = transform_point_hdiv( cfg->mProjInv, make_float4( pxm/(float)uWindowWidthEvenlyDivisibleByTileRes*2.f-1.f, (uWindowHeightEvenlyDivisibleByTileRes-pym)/(float)uWindowHeightEvenlyDivisibleByTileRes*2.f-1.f,1.f,1.f) );
        frustum[1] = transform_point_hdiv( cfg->mProjInv, make_float4( pxp/(float)uWindowWidthEvenlyDivisibleByTileRes*2.f-1.f, (uWindowHeightEvenlyDivisibleByTileRes-pym)/(float)uWindowHeightEvenlyDivisibleByTileRes*2.f-1.f,1.f,1.f) );
        frustum[2] = transform_point_hdiv( cfg->mProjInv, make_float4( pxp/(float)uWindowWidthEvenlyDivisibleByTileRes*2.f-1.f, (uWindowHeightEvenlyDivisibleByTileRes-pyp)/(float)uWindowHeightEvenlyDivisibleByTileRes*2.f-1.f,1.f,1.f) );
        frustum[3] = transform_point_hdiv( cfg->mProjInv, make_float4( pxm/(float)uWindowWidthEvenlyDivisibleByTileRes*2.f-1.f, (uWindowHeightEvenlyDivisibleByTileRes-pyp)/(float)uWindowHeightEvenlyDivisibleByTileRes*2.f-1.f,1.f,1.f) );*/

		float l = 2.0 *((float)tile_idx.x / num_tiles.x) - 1.0;
		float r = 2.0 * ((float)(tile_idx.x + 1) / num_tiles.x) - 1.0;
		float t = 2.0 * ((float)(tile_idx.y + 1) / num_tiles.y) - 1.0;
		float b = 2.0 * ((float)tile_idx.y / num_tiles.y) - 1.0;

		frustum[0] = transform_point_hdiv( cfg->mProjInv, make_float4( l, b,  1.f,  1.f ) );
        frustum[1] = transform_point_hdiv( cfg->mProjInv, make_float4( l, t,  1.f,  1.f ) );
        frustum[2] = transform_point_hdiv( cfg->mProjInv, make_float4( r, t,  1.f,  1.f ) );
        frustum[3] = transform_point_hdiv( cfg->mProjInv, make_float4( r, b,  1.f,  1.f ) );


        // create plane equations for the four sides of the frustum, 
        // with the positive half-space outside the frustum (and remember, 
        // view space is left handed, so use the left-hand rule to determine 
        // cross product direction)
        for(uint i=0; i<4; i++)
            frustumEqn[i] = create_plane( frustum[i], frustum[(i+1)&3] );
    }

	//	//int2 tex_coord;
	//	//tex_coord.x = (int)((p12.x / p12.w * 0.5 + 0.5) * 800);
	//	//tex_coord.y = (int)((- p12.y / p12.w * 0.5 + 0.5) * 600);
	//	//float depth = read_imagef(depthmap, tex_coord).x;
	float halfZ = 0.5 * (minmax.x + minmax.y);
	float r = bsphere.w;
	float4 center = bsphere;
	center.w = 1;
	center = transform_point_hdiv(cfg->mView, center); 
	if( ( sdist( center.xyz, frustumEqn[0] ) < r ) &&
            ( sdist( center.xyz, frustumEqn[1] ) < r ) &&
            ( sdist( center.xyz, frustumEqn[2] ) < r ) &&
            ( sdist( center.xyz, frustumEqn[3] ) < r ) )
        {
           // if( center.z + minmax.x < r && center.z - halfZ < r )
            //{
                // do a thread-safe increment of the list counter 
                // and put the index of this light into the list
				return true;
            //}

            //if( -center.z + halfZ < r && center.z - minmax.y < r )
            //{
				//return true;
            //}
       }

		return false;
}

__kernel void check_visibility(
						__global config* cfg,
						 uint num_bounds,
						__global float4* bounds,
						__read_only image2d_t depthmap,
						__global uint* visiblity
						)
{
	__local uint l_min;
	__local uint l_max;

	int2 local_id = make_int2(get_global_id(0), get_global_id(1));
	int2 group_id = make_int2(get_group_id(0), get_group_id(1));
	int2 group_dim = make_int2(get_local_size(0), get_local_size(1));

	int flat_local_id = local_id.y * group_dim.x + local_id.x;

	if (local_id.x == 0 && local_id.y == 0)
	{
		l_min = 10000.0;
		l_max = 0.0;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	int2 tex_coord = make_int2(group_id.x * TILE_SIZE_X + local_id.x, group_id.y * TILE_SIZE_Y + local_id.y);

	if (tex_coord.x < get_image_width(depthmap) && tex_coord.y < get_image_height(depthmap))
	{
		float depth = read_imagef(depthmap, tex_coord).x;
		atom_min(&l_min, as_uint(depth));
		atom_max(&l_max, as_uint(depth));
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = flat_local_id; i < num_bounds; i += group_dim.x * group_dim.y)
	{
		int2 num_tiles = make_int2(cfg->output_width / TILE_SIZE_X, cfg->output_height / TILE_SIZE_Y); 
		if (test_bsphere(cfg, bounds[i], group_id, make_int2(TILE_SIZE_X, TILE_SIZE_Y), num_tiles, make_float2(as_float(l_min), as_float(l_max))))
		{
			atom_max(&visiblity[i], 1);
		}
	}
}

/// replace with scan further
__kernel void build_command_list( uint num_bounds, 
						__global offset* offsets,  
						__global DrawElementsIndirectCommand* commands, 
						__global int* counter,
						__global uint* visibility)
{
	uint global_id = get_global_id(0);

	if (global_id < num_bounds)
	{
		if (visibility[global_id])
		{
			int idx = atom_inc(counter);

			__global DrawElementsIndirectCommand* cmd = commands + idx;

			cmd->count = offsets[global_id].num_idx;
			cmd->instanceCount = 1;
			cmd->firstIndex= offsets[global_id].start_idx;
			cmd->baseVertex = 0;
			cmd->baseInstance = 0;
		}
	}
}


