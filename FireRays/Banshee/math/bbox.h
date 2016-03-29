/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/
#ifndef BBOX_H
#define BBOX_H

#include <cmath>
#include <algorithm>
#include <limits>

#include "float3.h"
#include "ray.h"

class bbox
{
public:
    bbox()
        : pmin(float3(std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max()))
        , pmax(float3(-std::numeric_limits<float>::max(),
                      -std::numeric_limits<float>::max(),
                      -std::numeric_limits<float>::max()))
    {
    }

    bbox(float3 const& p)
        : pmin(p)
        , pmax(p)
    {
    }

    bbox(float3 const& p1, float3 const& p2)
        : pmin(vmin(p1, p2))
        , pmax(vmax(p1, p2))
    {
    }

    float3 center()  const { return 0.5f * (pmax + pmin); }
    float3 extents() const { return pmax - pmin; }

    bool  contains(float3 const& p) const;

    int   maxdim() const;
    float surface_area() const;

    // TODO: this is non-portable, optimization trial for fast intersection test
    float3 const& operator [] (int i) const { return *(&pmin + i); }

    // Grow the bounding box by a point
    void grow(float3 const& p);
    // Grow the bounding box by a box
    void grow(bbox const& b);


    float3 pmin;
    float3 pmax;
};

inline bool   bbox::contains(float3 const& p) const
{
    float3 radius = 0.5f * extents();
    return std::abs(center().x - p.x) <= radius.x &&
        fabs(center().y - p.y) <= radius.y &&
        fabs(center().z - p.z) <= radius.z;
}

inline bbox bboxunion(bbox const& box1, bbox const& box2)
{
    bbox res;
    res.pmin = vmin(box1.pmin, box2.pmin);
    res.pmax = vmax(box1.pmax, box2.pmax);
    return res;
}

inline bbox intersection(bbox const& box1, bbox const& box2)
{
    return bbox(vmax(box1.pmin, box2.pmin), vmin(box1.pmax, box2.pmax));
}

inline void intersection(bbox const& box1, bbox const& box2, bbox& box)
{
    vmax(box1.pmin, box2.pmin, box.pmin);
    vmin(box1.pmax, box2.pmax, box.pmax);
}

inline void bbox::grow(float3 const& p)
{
    vmin(pmin, p, pmin);
    vmax(pmax, p, pmax);
}

inline void bbox::grow(bbox const& b)
{
    vmin(pmin, b.pmin, pmin);
    vmax(pmax, b.pmax, pmax);
}

#define BBOX_INTERSECTION_EPS 0.f

inline bool intersects(bbox const& box1, bbox const& box2)
{
    float3 b1c = box1.center();
    float3 b1r = 0.5f * box1.extents();
    float3 b2c = box2.center();
    float3 b2r = 0.5f * box2.extents();

    return (fabs(b2c.x - b1c.x) - (b1r.x + b2r.x)) <= BBOX_INTERSECTION_EPS &&
           (fabs(b2c.y - b1c.y) - (b1r.y + b2r.y)) <= BBOX_INTERSECTION_EPS &&
           (fabs(b2c.z - b1c.z) - (b1r.z + b2r.z)) <= BBOX_INTERSECTION_EPS;
}

inline bool contains(bbox const& box1, bbox const& box2)
{
    return box1.contains(box2.pmin) && box1.contains(box2.pmax);
}

inline bool intersects(ray const& r, float3 const& invrd, bbox const& box)
{
    float2 tt = r.t;
    
    for (int i = 0; i<3; ++i)
    {
        float tn = (box.pmin[i] - r.o[i]) * invrd[i];
        float tf = (box.pmax[i] - r.o[i]) * invrd[i];
        if (tn > tf) std::swap(tn, tf);
        tt.x = tn > tt.x ? tn : tt.x;
        tt.y = tf < tt.y ? tf : tt.y;
        
        if (tt.x > tt.y) return false;
    }
    
    return true;
}

// Fast bbox test: PBRT book
inline bool intersects(ray const& r, float3 const& invrd, bbox const& box, int dirneg[3], float maxt)
{
    // Check for ray intersection against $x$ and $y$ slabs
    float tmin =  (box[  dirneg[0]].x - r.o.x) * invrd.x;
    float tmax =  (box[1-dirneg[0]].x - r.o.x) * invrd.x;
    float tymin = (box[  dirneg[1]].y - r.o.y) * invrd.y;
    float tymax = (box[1-dirneg[1]].y - r.o.y) * invrd.y;
    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    // Check for ray intersection against $z$ slab
    float tzmin = (box[  dirneg[2]].z - r.o.z) * invrd.z;
    float tzmax = (box[1-dirneg[2]].z - r.o.z) * invrd.z;
    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;
    return (tmin < maxt) && (tmax > r.t.x);
}

// Fast bbox test: PBRT book
inline bool intersects(ray const& r, float3 const& invrd, bbox const& box, int dirneg[3], float2& range)
{
    // Check for ray intersection against $x$ and $y$ slabs
    float tmin =  (box[  dirneg[0]].x - r.o.x) * invrd.x;
    float tmax =  (box[1-dirneg[0]].x - r.o.x) * invrd.x;
    float tymin = (box[  dirneg[1]].y - r.o.y) * invrd.y;
    float tymax = (box[1-dirneg[1]].y - r.o.y) * invrd.y;
    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;
    
    // Check for ray intersection against $z$ slab
    float tzmin = (box[  dirneg[2]].z - r.o.z) * invrd.z;
    float tzmax = (box[1-dirneg[2]].z - r.o.z) * invrd.z;
    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;
    
    if ((tmin < r.t.y) && (tmax > r.t.x))
    {
        range = float2(std::max(r.t.x, tmin), std::min(r.t.y, tmax));
        return true;
    }
    
    return false;
}

inline int bbox::maxdim() const
{
    float3 ext = extents();
    
    if (ext.x >= ext.y && ext.x >= ext.z)
        return 0;
    if (ext.y >= ext.x && ext.y >= ext.z)
        return 1;
    if (ext.z >= ext.x && ext.z >= ext.y)
        return 2;
    
    return 0;
}

inline float bbox::surface_area() const
{
    float3 ext = extents();
    return 2.f * (ext.x * ext.y + ext.x * ext.z + ext.y * ext.z);
}

#endif // BBOX_H
