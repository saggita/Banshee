/// Extensions
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable
 

//#define TRAVERSAL_STACKLESS

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

	if (temp > 0 && temp < r->maxt)
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

	if (hit && t < r->maxt)
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
	float depth = DEFAULT_DEPTH;
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

#define NODE_STACK_SIZE 64
#define NODE_STACK_INIT(stack) \
    __local int* stack_ptr = stack; \
    *stack_ptr++ = -1
#define NODE_STACK_PUSH(val) *stack_ptr++ = (val)
#define NODE_STACK_POP *--stack_ptr
#define NODE_STACK_GUARD -1

// intersect ray against the whole BVH structure
bool traverse_bvh_stacked(__local int* stack, __global bvh_node* nodes, __global float4* vertices, __global uint4* indices, ray* r)
{
    // init node stack
    NODE_STACK_INIT(stack);
    
    float depth = DEFAULT_DEPTH;
    bool hit = false;
    
    // start from the root
    uint idx = 0;
    do
    {
        float tt = DEFAULT_DEPTH;
        
        // load current node
        bbox box = nodes[idx].box;
        uint prim_start_index = nodes[idx].prim_start_index;
        uint  left  = idx + 1;
        uint  right = nodes[idx].right;
        uint  num_prims = nodes[idx].num_prims;
        
        // try intersecting against current node's bounding box
        if (intersect_box(r, box))
        {
            // if this is the leaf try to intersect against contained triangle
            if (num_prims != 0)
            {
                hit |= intersect_leaf(vertices, indices, prim_start_index, num_prims, r);
            }
            // traverse child nodes otherwise
            else
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
				continue;
            }
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
#ifndef TRAVERSAL_STACKLESS
	__local int stack[64 * 64];
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

#ifdef TRAVERSAL_STACKLESS
		bool hit  = traverse_bvh_stackless(bvh, vertices, indices, &rr);
#else
		__local int* this_thread_stack = stack + 64 * flat_local_id;
		bool hit  = traverse_bvh_stacked(this_thread_stack, bvh, vertices, indices, &rr);
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

bool test_bsphere(__global float4* bounds, int idx, float16 mvp, int2 tile_idx, int2 tile_dim, float2 minmax)
{
		//float4 pmin = make_float4(bounds[idx].pmin.x, bounds[idx].pmin.y, bounds[idx].pmin.z, 1);
		//float4 pmax = make_float4(bounds[idx].pmax.x, bounds[idx].pmax.y, bounds[idx].pmax.z, 1);

		//float4 p0 = make_float4(pmin.x, pmin.y, pmax.z, 1);
		//float4 p1 = make_float4(pmin.x, pmax.y, pmax.z, 1);

		//float4 p2 = make_float4(pmax.x, pmin.y, pmax.z, 1);
		//float4 p3 = make_float4(pmin.x, pmax.y, pmin.z, 1);

		//float4 p4 = make_float4(pmax.x, pmin.y, pmin.z, 1);
		//float4 p5 = make_float4(pmax.x, pmax.y, pmin.z, 1);

		//pmin = transform_point(mvp, pmin);
		//pmax = transform_point(mvp, pmax);

		//p0 = transform_point(mvp, p0);
		//p1 = transform_point(mvp, p1);
		//p2 = transform_point(mvp, p2);
		//p3 = transform_point(mvp, p3);
		//p4 = transform_point(mvp, p4);
		//p5 = transform_point(mvp, p5);

		//bool inside = frustum_check(pmin) || frustum_check(pmax) || frustum_check(p0)
		//	|| frustum_check(p1) || frustum_check(p2) || frustum_check(p3) 
		//	|| frustum_check(p4) || frustum_check(p5);

		//		float4 p6 = find_min_z(pmin,pmax);
		//float4 p7 = find_min_z(p0,p1);
		//float4 p8 = find_min_z(p2,p3);
		//float4 p9 = find_min_z(p4,p5);
		//float4 p10 = find_min_z(p6,p7);
		//float4 p11 = find_min_z(p8,p9);
		//float4 p12 = find_min_z(p10,p11);

		//int2 tex_coord;
		//tex_coord.x = (int)((p12.x / p12.w * 0.5 + 0.5) * 800);
		//tex_coord.y = (int)((- p12.y / p12.w * 0.5 + 0.5) * 600);
		//float depth = read_imagef(depthmap, tex_coord).x;

		return true;
}

__kernel void check_visibility(
						float16 mvp, 
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
		if (test_bsphere(bounds, i, mvp, group_id, make_int2(TILE_SIZE_X, TILE_SIZE_Y), make_float2(as_float(l_min), as_float(l_max))))
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


