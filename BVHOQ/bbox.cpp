//
//  bbox.cpp
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "bbox.h"
bbox::bbox(vector3 const& p)
: pmin_(p)
, pmax_(p)
{
    
}

bbox::bbox(vector3 const& p1, vector3 const& p2)
: pmin_(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()), std::min(p1.z(), p2.z()))
, pmax_(std::max(p1.x(), p2.x()), std::max(p1.y(), p2.y()), std::max(p1.z(), p2.z()))
{
}

vector3& bbox::max()
{
    return pmax_;
}

vector3& bbox::min()
{
    return pmin_;
}

vector3 const& bbox::max() const
{
    return pmax_;
}

vector3 const& bbox::min() const
{
    return pmin_;
}

vector3 bbox::extents() const
{
    return pmax_ - pmin_;
}

vector3 bbox::center() const
{
    return 0.5f * (pmax_ + pmin_);
}

bool bbox::contains(vector3 const& p) const
{
    vector3 radius = 0.5f * extents();
    return abs(center().x() - p.x()) < radius.x() &&
           abs(center().y() - p.y()) < radius.y() &&
           abs(center().z() - p.z()) < radius.z();
}

int bbox::max_extent() const
{
    vector3 ext = extents();
    
    if (ext.x() >= ext.y() && ext.x() >= ext.z())
        return 0;
    if (ext.y() >= ext.x() && ext.y() >= ext.z())
        return 1;
    if (ext.z() >= ext.x() && ext.z() >= ext.y())
        return 2;
    
    return 0;
}

float bbox::surface_area() const
{
    return 2 * (extents().x() * extents().y() + extents().x() * extents().z() + extents().y() * extents().z());
}
