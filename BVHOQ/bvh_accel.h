//
//  bvh_accel.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__bvh_accel__
#define __BVHOQ__bvh_accel__

#include "common_types.h"
#include "scene_base.h"
#include "bbox.h"

#include <list>
#include <memory>

class bvh_accel
{
public:
    static std::shared_ptr<bvh_accel> create_from_scene(scene_base const& scene);
    
    bvh_accel(std::vector<vector3> const& vertices, std::vector<unsigned> const& indices, unsigned max_node_prims);
    ~bvh_accel();
    
    struct node
    {
        bbox box;
        unsigned prim_start_index;
        unsigned right;
        unsigned parent;
        unsigned num_prims;
    };
    
    struct triangle
    {
        unsigned i1,i2,i3,padding;
    };
    
    std::vector<vector3> const& vertices() const;
    std::vector<triangle> const& primitives() const;
    std::vector<node> const& nodes() const;
    int root_node() const;
    
    bool intersect(ray const& r, float& t);
    
private:
    struct build_node
    {
        bbox box;
        unsigned prim_start_index;
        unsigned left;
        unsigned right;
        unsigned parent;
        unsigned num_prims;
    };

	struct build_info
	{
		bbox box;
		unsigned primitive_index;
	};

	unsigned create_leaf_node(std::vector<build_node>& build_nodes, unsigned begin, unsigned end, bbox const& b);
    unsigned find_best_split(unsigned begin, unsigned end, bbox const& box, bbox const& centroids_bbox, float& sah_val);
    unsigned build_hierarchy(unsigned begin, unsigned end, std::vector<build_node>& build_nodes);
    unsigned linearize(unsigned build_node_idx, unsigned parent, std::vector<build_node> const& build_nodes);
    bool intersect(int idx, ray const& r, float& t);
    
    bbox triangle_bbox(triangle const& t);
    
    // temporary solution to keep geometry here
    std::vector<vector3> vertices_;
    std::vector<triangle> primitives_reordered_;
    std::vector<triangle> primitives_;
	std::vector<build_info> build_info_;
    std::vector<node> nodes_;
    
	unsigned max_node_prims_;
    int root_;
};

#endif /* defined(__BVHOQ__bvh_accel__) */
