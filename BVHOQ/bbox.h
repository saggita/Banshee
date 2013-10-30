//
//  bbox.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__bbox__
#define __BVHOQ__bbox__

#include <iostream>
#include <algorithm>
#include "common_types.h"

class bbox
{
public:
    bbox(vector3 const& p = vector3());
    bbox(vector3 const& pmin, vector3 const& pmax);
    
    vector3& max();
    vector3& min();
    
    vector3 const& max() const;
    vector3 const& min() const;
    
    vector3 center() const;
    vector3 extents() const;
    
    bool contains(vector3 const& p) const;
    
    int max_extent() const;
    float surface_area() const;
    
private:
    vector3 pmin_;
    vector3 pmax_;
};

inline bbox bbox_union(bbox const& box1, bbox const& box2)
{
    vector3 pmin(std::min(box1.min().x(), box2.min().x()), std::min(box1.min().y(), box2.min().y()), std::min(box1.min().z(), box2.min().z()));
    vector3 pmax(std::max(box1.max().x(), box2.max().x()), std::max(box1.max().y(), box2.max().y()), std::max(box1.max().z(), box2.max().z()));
    
    return bbox(pmin, pmax);
}

inline bool intersects(bbox const& box1, bbox const& box2)
{
    vector3 box1_center = box1.center();
    vector3 box1_radius = 0.5f * box1.extents();
    vector3 box2_center = box2.center();
    vector3 box2_radius = 0.5f * box2.extents();
    
    return abs(box2_center.x() - box1_center.x()) < box1_radius.x() + box2_radius.x() &&
           abs(box2_center.y() - box1_center.y()) < box1_radius.y() + box2_radius.y() &&
           abs(box2_center.x() - box1_center.z()) < box1_radius.z() + box2_radius.z();
}

#endif /* defined(__BVHOQ__bbox__) */
