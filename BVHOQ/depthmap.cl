/// Extensions
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

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
bool intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float* t)
{
	float3 e1 = sub(v2, v1);
	float3 e2 = sub(v3, v1);

	float3 s1 = cross(r.d, e2);
	float inv_div = 1.0/dot(s1, e1);

	//if (div == 0.f)
	//return false;

	float3 d = r.o - v1;
	float b1 = dot(d,s1) * inv_div;

	if (b1 < 0 || b1 > 1)
		return false;

	float3 s2 = cross(d, e1);
	float b2 = dot(r.d, s2) * inv_div;

	if (b2 < 0 || b1 + b2 > 1)
		return false;

	float temp = dot(e2, s2) * inv_div;

	if (t && temp >= 0)
	{
		*t = temp;
		return true;
	}

	return false;
}

// intersect ray with the axis-aligned box
bool intersect_box(ray r, bbox box, float* t)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	float3 bounds[2];
	bounds[0] = box.pmin;
	bounds[1] = box.pmax;

	if (r.d.x >= 0) {
		tmin = (bounds[0].x - r.o.x) / r.d.x;
		tmax = (bounds[1].x - r.o.x) / r.d.x;
	} else {
		tmin = (bounds[1].x - r.o.x) / r.d.x;
		tmax = (bounds[0].x - r.o.x) / r.d.x;
	}
	if (r.d.y >= 0) {
		tymin = (bounds[0].y - r.o.y) / r.d.y;
		tymax = (bounds[1].y - r.o.y) / r.d.y;
	} else {
		tymin = (bounds[1].y - r.o.y) / r.d.y;
		tymax = (bounds[0].y - r.o.y) / r.d.y;
	}
	if ( (tmin > tymax) || (tymin > tmax) )
		return false;

	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;
	if (r.d.z >= 0) {
		tzmin = (bounds[0].z - r.o.z) / r.d.z;
		tzmax = (bounds[1].z - r.o.z) / r.d.z;
	} else {
		tzmin = (bounds[1].z - r.o.z) / r.d.z;
		tzmax = (bounds[0].z - r.o.z) / r.d.z;
	}
	if ( (tmin > tzmax) || (tzmin > tmax) )
		return false;

	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	if (tmin < 0 && tmax < 0)
		return false;

	if (t)
	{
		*t = tmin >=0 ? tmin : tmax;
	}

	return true;
}

/// BVH related functions
//  intersect a ray with BVH leaf
bool intersect_leaf(__global float4* vertices, __global uint4* indices, uint idx, int num_prims, ray r, float* t)
{
	bool hit = false;
	float depth = DEFAULT_DEPTH;
	for (int i = 0; i < num_prims; ++i)
	{
		uint4 triangle = indices[idx + i];

		float4 v1 = vertices[triangle.x];
		float4 v2 = vertices[triangle.y];
		float4 v3 = vertices[triangle.z];

		float tt = DEFAULT_DEPTH;
		if (intersect_triangle(r, v1.xyz, v2.xyz, v3.xyz, &tt))
		{
			hit = true;
			depth = min(depth, tt);
		}
	}

	if (hit && t)
	{
		*t = depth;
	}

	return hit;
}

#define NODE_STACK_SIZE 64
#define NODE_STACK_INIT(size) \
	int stack[size]; \
	int* stack_ptr = stack; \
	*stack_ptr++ = -1
#define NODE_STACK_PUSH(val) *stack_ptr++ = (val)
#define NODE_STACK_POP *--stack_ptr
#define NODE_STACK_GUARD -1

// intersect ray against the whole BVH structure
bool traverse_bvh_stacked(__global bvh_node* nodes, __global float4* vertices, __global uint4* indices, ray r, float* t)
{
	// init node stack
	NODE_STACK_INIT(NODE_STACK_SIZE);

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
		if (intersect_box(r, box, &tt))
		{
			// if this is the leaf try to intersect against contained triangle
			if (prim_start_index != 0xffffffff)
			{
#ifndef TRACE_INTERNAL_ONLY
				if (intersect_leaf(vertices, indices, prim_start_index, num_prims, r, &tt))
				{
#endif
					depth = min(depth, tt);
					hit = true;
#ifndef TRACE_INTERNAL_ONLY
				}
#endif
			}
			// traverse child nodes otherwise
			else
			{
				NODE_STACK_PUSH(right);
				idx = left;
				continue;
			}
		}

		// pop next item from the stack
		idx = NODE_STACK_POP;
	}
	while (idx != NODE_STACK_GUARD);

	// return depth
	if (t && hit)
	{
		*t = depth;
	}

	return hit;
}

#define FROM_PARENT  1
#define FROM_CHILD   2
#define FROM_SIBLING 3

#define PARENT(x) (nodes[(x)].parent)
#define LEFT_CHILD(x) ((x)+1)
#define RIGHT_CHILD(x) (nodes[(x)].right)

#define IS_LEAF(x) (nodes[(x)].prim_start_index != 0xffffffff)

bool traverse_bvh_stackless(__global bvh_node* nodes, __global float4* vertices, __global uint4* indices, ray r, float* t)
{
	uint current = 1;
	int  state   = FROM_PARENT;

	bool hit = false;
	float depth = DEFAULT_DEPTH;

	while (true)
	{
		switch (state)
		{
		case FROM_CHILD:
			{
				if (current == 0)
				{
					if (hit && t)
					{
						*t = depth;
					}

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
				float tt = DEFAULT_DEPTH;
				bbox box = nodes[current].box;
				if (!intersect_box(r, box, &tt))
				{
					current = PARENT(current);
					state = FROM_CHILD;
				}
				else if (IS_LEAF(current))
				{
					if (intersect_leaf(vertices, indices, nodes[current].prim_start_index, nodes[current].num_prims, r, &tt))
					{
						depth = min(depth, tt);
						hit = true;
					}

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
				float tt = DEFAULT_DEPTH;
				bbox box = nodes[current].box;
				if (!intersect_box(r, box, &tt))
				{
					current = RIGHT_CHILD(PARENT(current));
					state = FROM_SIBLING;
				}
				else if (IS_LEAF(current))
				{
					if (intersect_leaf(vertices, indices, nodes[current].prim_start_index, nodes[current].num_prims, r, &tt))
					{
						depth = min(depth, tt);
						hit = true;
					}

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
__kernel void k(__global bvh_node* bvh,
				__global float4* vertices,
				__global uint4* indices,
				__global config* params,
				__write_only image2d_t output)
{
	int2 global_id;
	global_id.x = get_global_id(0);
	global_id.y = get_global_id(1);

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

		float res = DEFAULT_DEPTH;
		float depth = DEFAULT_DEPTH;

		if (traverse_bvh_stackless(bvh, vertices, indices, rr, &depth))
		{
			depth /= 25.0;
			res = depth;
		}

		write_imagef(output, global_id, res);
	}
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

__kernel void bbox_cull(float16 mvp, 
						uint num_bounds, 
						__global bbox* bounds, 
						__global offset* offsets,  
						__read_only image2d_t depthmap, 
						__global DrawElementsIndirectCommand* commands, 
						__global int* counter)
{
	uint global_id = get_global_id(0);

	if (global_id < num_bounds)
	{
		float4 pmin = make_float4(bounds[global_id].pmin.x, bounds[global_id].pmin.y, bounds[global_id].pmin.z, 1);
		float4 pmax = make_float4(bounds[global_id].pmax.x, bounds[global_id].pmax.y, bounds[global_id].pmax.z, 1);

		float4 p0 = make_float4(pmin.x, pmin.y, pmax.z, 1);
		float4 p1 = make_float4(pmin.x, pmax.y, pmax.z, 1);

		float4 p2 = make_float4(pmax.x, pmin.y, pmax.z, 1);
		float4 p3 = make_float4(pmin.x, pmax.y, pmin.z, 1);

		float4 p4 = make_float4(pmax.x, pmin.y, pmin.z, 1);
		float4 p5 = make_float4(pmax.x, pmax.y, pmin.z, 1);

		pmin = transform_point(mvp, pmin);
		pmax = transform_point(mvp, pmax);

		p0 = transform_point(mvp, p0);
		p1 = transform_point(mvp, p1);
		p2 = transform_point(mvp, p2);
		p3 = transform_point(mvp, p3);
		p4 = transform_point(mvp, p4);
		p5 = transform_point(mvp, p5);

		bool inside = frustum_check(pmin) || frustum_check(pmax) || frustum_check(p0)
			|| frustum_check(p1) || frustum_check(p2) || frustum_check(p3) 
			|| frustum_check(p4) || frustum_check(p5);

		if (inside)
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
