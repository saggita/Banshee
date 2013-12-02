//
//  bvh_accel.cpp
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "bvh_accel.h"

#include <algorithm>

bool intersect(ray const& r, bbox const& box, float& t)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	vector3 bounds[2] = { box.min(), box.max() };
	
	if (r.d.x() >= 0) {
		tmin = (bounds[0].x() - r.o.x()) / r.d.x();
		tmax = (bounds[1].x() - r.o.x()) / r.d.x();
	} else {
		tmin = (bounds[1].x() - r.o.x()) / r.d.x();
		tmax = (bounds[0].x() - r.o.x()) / r.d.x();
	}
	if (r.d.y() >= 0) {
		tymin = (bounds[0].y() - r.o.y()) / r.d.y();
		tymax = (bounds[1].y() - r.o.y()) / r.d.y();
	} else {
		tymin = (bounds[1].y() - r.o.y()) / r.d.y();
		tymax = (bounds[0].y() - r.o.y()) / r.d.y();
	}
	if ( (tmin > tymax) || (tymin > tmax) )
		return false;
	
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;
	if (r.d.z() >= 0) {
		tzmin = (bounds[0].z() - r.o.z()) / r.d.z();
		tzmax = (bounds[1].z() - r.o.z()) / r.d.z();
	} else {
		tzmin = (bounds[1].z() - r.o.z()) / r.d.z();
		tzmax = (bounds[0].z() - r.o.z()) / r.d.z();
	}
	if ( (tmin > tzmax) || (tzmin > tmax) )
		return false;
	
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	
	t = tmin;
	
	if (t < 0 && tmax < 0) return  false;
	else t = tmax;
	
	return true;
}

bool intersect(ray const& r, vector3 const& v1, vector3 const& v2, vector3 const& v3, float& t)
{
	vector3 e1 = v2 - v1;
	vector3 e2 = v3 - v1;
	vector3 s1 = cross(r.d, e2);
	float div = dot(s1, e1);
	
	if (div == 0.f)
		return false;
	
	vector3 d = r.o - v1;
	float b1 = dot(d,s1) / div;
	
	if (b1 < 0 || b1 > 1)
		return false;
	
	vector3 s2 = cross(d, e1);
	float b2 = dot(r.d, s2) / div;
	
	if (b2 < 0 || b1 + b2 > 1)
		return false;
	
	t = dot(e2, s2) / div;
	
	if (t < 0)
	{
		return false;
	}
	
	return true;
}

float surface_area(vector3 const& v1, vector3 const& v2, vector3 const& v3)
{
	vector3 v12 = v2 - v1;
	vector3 v13 = v3 - v1;
	
	return 0.5f * sqrtf(cross(v12, v13).sq_norm());
	
}

bbox bvh_accel::triangle_bbox(triangle const& t)
{
	bbox b(vertices_[t.i1], vertices_[t.i2]);
	return bbox_union(b, vertices_[t.i3]);
}

std::shared_ptr<bvh_accel> bvh_accel::create_from_scene(scene_base const& scene)
{
	return std::make_shared<bvh_accel>(scene.vertices(), scene.indices(), 255);
}

template <typename T> bvh_accel::bvh_accel(std::vector<T> const& vertices, std::vector<unsigned> const& indices, unsigned max_node_prims)
	: max_node_prims_(max_node_prims)
{
	vertices_.resize(vertices.size());
	std::transform(vertices.begin(), vertices.end(), vertices_.begin(), 
		[](T const& v)
	{
		return v.position;
	});

	/// build primitives list
	primitives_.resize(indices.size()/3);
	build_info_.resize(indices.size()/3);
	for (unsigned i = 0; i < indices.size(); i += 3)
	{
		bbox b(vertices_[indices[i]], vertices_[indices[i+1]]);
		b = bbox_union(b, vertices_[indices[i+2]]);
	
		triangle t = {indices[i], indices[i + 1], indices[i + 2]};
		primitives_[i/3] = t;

		build_info info = { b, i/3 };
		build_info_[i/3] = info;
	}

	std::vector<build_node> build_nodes;
	root_ = build_hierarchy(0, (unsigned)build_info_.size(), build_nodes);

	/// this is important to avoid nodes_ reallocation
	/// while recursing down the structure
	nodes_.reserve(build_nodes.size());
	linearize(root_, 0, build_nodes);
	std::cout << "BVH contains " << nodes_.size() << " nodes\n";
}


bvh_accel::~bvh_accel()
{
}

unsigned bvh_accel::create_leaf_node(std::vector<build_node>& build_nodes, unsigned begin, unsigned end, bbox const& b)
{
	// reorder primitives to have only start ptr and size in a leaf
	unsigned leaf_begin = (unsigned)primitives_reordered_.size();
	for (unsigned i = begin; i < end; ++i)
	{
		primitives_reordered_.push_back(primitives_[build_info_[i].primitive_index]);
	}

	build_node n = {b, leaf_begin, 0, 0, 0, (unsigned)(end - begin)};
	build_nodes.push_back(n);
	return (unsigned)build_nodes.size() - 1;
}


unsigned bvh_accel::build_hierarchy(unsigned begin, unsigned end, std::vector<build_node>& build_nodes)
{
	// calculate node bbox and centroids bounds to find maximum spatial extent
	bbox b = build_info_[begin].box;
	bbox centroids_bbox(b.center());

	std::for_each(&build_info_[0] + begin, &build_info_[0] + end, [&b, &centroids_bbox, this](build_info const& info)
				  {
					  b = bbox_union(b, info.box);
					  centroids_bbox = bbox_union(centroids_bbox, info.box.center());
				  });

	// axis to sort along
	float sah_val;
	int num_prims = end - begin;
	int axis = centroids_bbox.max_extent();
	int mid = 0;

	if (centroids_bbox.min()[axis] == centroids_bbox.max()[axis])
	{
		if (num_prims <= max_node_prims_)
		{
			return create_leaf_node(build_nodes, begin, end, b);
		}
		else
		{
			mid = (begin + end)/2;
		}
	}
	else
	{
		mid = find_best_split(begin, end, b, centroids_bbox, sah_val);
		assert(mid >= begin && mid <= end);
	}
	
	// Decide whether to split or to create new leaf
	if (num_prims > max_node_prims_ || (sah_val > num_prims && num_prims > 1))
	{
		build_node n = {b, axis, 0, 0, 0};
	
		//unsigned mid = find_best_split(begin, end, b, centroids_bbox);
		n.left = build_hierarchy(begin, mid, build_nodes);
		n.right = build_hierarchy(mid, end, build_nodes);
	
		build_nodes[n.left].parent = (unsigned)build_nodes.size();
		build_nodes[n.right].parent = (unsigned)build_nodes.size();
 
		build_nodes.push_back(n);
 
		return (unsigned)build_nodes.size() - 1;
	}
	else
	{
		return create_leaf_node(build_nodes, begin, end, b);
	}
}

unsigned   bvh_accel::find_best_split(unsigned begin, unsigned end, bbox const& box, bbox const& centroids_bbox, float& sah_val)
{
	// SAH implementation
	// calc centroids histogram
	unsigned const NUM_BINS = 10;
	
	if (end - begin < NUM_BINS)
	{
		return (begin + end)/2;
	}
	
	struct bin
	{
		bbox box;
		unsigned count;
	};
	
	bin bins[NUM_BINS];
	
	int dim = centroids_bbox.max_extent();
	
	float centroid_range = centroids_bbox.extents()[dim];
	float bin_range = centroid_range / NUM_BINS;
	
	for (unsigned i = 0; i < NUM_BINS; ++i)
	{
		vector3 d = vector3(0,0,0);
		d[dim] = i * bin_range;
		bins[i].count = 0;
		bins[i].box = bbox(centroids_bbox.min() + d);
	}
	
	for (unsigned i = begin; i < end; ++i)
	{
		bbox b = build_info_[i].box;
		unsigned bin_idx = (unsigned)std::min<float>(NUM_BINS * ((b.center()[dim] - centroids_bbox.min()[dim]) / centroid_range), NUM_BINS-1);
		
		assert(bin_idx >= 0);
		
		++bins[bin_idx].count;
		bins[bin_idx].box = bbox_union(bins[bin_idx].box, b);
	}
	
	float sah_cost[NUM_BINS-1];
	
	for (unsigned i = 0; i < NUM_BINS - 1; ++i)
	{
		bbox half1_box = bins[0].box;
		unsigned half1_count = bins[0].count;
		
		for(unsigned j = 1; j <= i; ++j)
		{
			half1_box = bbox_union(half1_box, bins[j].box);
			half1_count += bins[j].count;
		}
		
		bbox half2_box = bins[i + 1].box;
		unsigned half2_count = bins[i + 1].count;
		
		for(unsigned j = i + 2; j < NUM_BINS; ++j)
		{
			half1_box = bbox_union(half1_box, bins[j].box);
			half1_count += bins[j].count;
		}
		
		sah_cost[i] = 500.0f + (half1_count * half1_box.surface_area() + half2_count * half2_box.surface_area())/box.surface_area();
	}
	
	float min_sah_cost = sah_cost[0];
	unsigned min_sah_split_idx = 0;
	
	for (unsigned i = 1; i < NUM_BINS - 1; ++i)
	{
		if (sah_cost[i] < min_sah_cost)
		{
			min_sah_cost = sah_cost[i];
			min_sah_split_idx = i;
		}
	}

	float border = bins[min_sah_split_idx + 1].box.center()[dim];
	auto iter = std::partition(&build_info_[0] + begin, &build_info_[0] + end, [=](build_info const& info)
	{
		return info.box.center()[dim] < border;
	});

	sah_val = min_sah_cost;

	return (unsigned)((iter - &build_info_[begin]) + begin);
}

std::vector<vector3> const& bvh_accel::vertices() const
{
	return vertices_;
}

std::vector<bvh_accel::triangle> const& bvh_accel::primitives() const
{
	return primitives_reordered_;
}

std::vector<bvh_accel::node> const& bvh_accel::nodes() const
{
	return nodes_;
}

int bvh_accel::root_node() const
{
	return root_;
}

bool  bvh_accel::intersect(int idx, ray const& r, float& t)
{
	float tt;
	node* n = &nodes_[idx];
	if (::intersect(r, n->box, tt))
	{
		if (n->prim_start_index != 0xffffffff)
		{
			float depth = std::numeric_limits<float>::max();
			float tt = std::numeric_limits<float>::max();
			bool hit = false;
			
			for (unsigned i = 0; i < n->num_prims; ++i)
			{
				if(::intersect(r, vertices_[primitives_reordered_[n->prim_start_index + i].i1],
							vertices_[primitives_reordered_[n->prim_start_index + i].i2], vertices_[primitives_reordered_[n->prim_start_index + i].i3], tt))
				{
					hit = true;
					depth = std::min(depth, tt);
				}
				
			}
			
			if (hit)
			{
				t = depth;
			}
			
			return hit;
		}
		else
		{
			bool hit1 = false;
			bool hit2 = false;
			float t1 = std::numeric_limits<float>::max();
			float t2 = std::numeric_limits<float>::max();
			
			hit1 = intersect(idx + 1, r, t1);
			hit2 = intersect(n->right, r, t2);
			
			if (hit1 || hit2)
			{
				t = std::min(t1,t2);
			}
			
			return hit1 || hit2;
		}
	}
	
	return false;
}

bool bvh_accel::intersect(ray const& r, float& t)
{
	bool hit = false;
	
	float depth = std::numeric_limits<float>::max();
	float tt = std::numeric_limits<float>::max();
	
	if (intersect(0, r, tt))
	{
		hit = true;
		depth = std::min(depth, tt);
	}
	
	if (hit)
	{
		t = depth;
	}
	
	return hit;
}

unsigned bvh_accel::linearize(unsigned build_node_idx, unsigned parent, std::vector<build_node> const& build_nodes)
{
	/// put the node into nodes_ list
	node n;
	n.box = build_nodes[build_node_idx].box;
	n.prim_start_index = build_nodes[build_node_idx].prim_start_index;
	n.num_prims = build_nodes[build_node_idx].num_prims;
	n.parent = parent;
	
	/// keep current index
	unsigned index = (unsigned)nodes_.size();
	nodes_.push_back(n);
	
	/// if this is internal node - linearize children nodes
	if (n.num_prims == 0)
	{
		linearize(build_nodes[build_node_idx].left, index, build_nodes);

		nodes_[index].right = linearize(build_nodes[build_node_idx].right, index, build_nodes);
	}
	
	return index;
}
