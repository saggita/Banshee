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

    bool   contains(float3 const& p) const;

    int   maxdim() const;
    float surface_area() const;

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
    return bbox(vmin(box1.pmin, box2.pmin), vmax(box1.pmax, box2.pmax));
}

inline bbox intersection(bbox const& box1, bbox const& box2)
{	
    return bbox(vmax(box1.pmin, box2.pmin), vmin(box1.pmax, box2.pmax));
}

inline bool intersects(bbox const& box1, bbox const& box2)
{
    float3 b1c = box1.center();
    float3 b1r = 0.5f * box1.extents();
    float3 b2c = box2.center();
    float3 b2r = 0.5f * box2.extents();

    return fabs(b2c.x - b1c.x) < b1r.x + b2r.x &&
        fabs(b2c.y - b1c.y) < b1r.y + b2r.y &&
        fabs(b2c.x - b1c.z) < b1r.z + b2r.z;
}

inline bool contains(bbox const& box1, bbox const& box2)
{
    return box1.contains(box2.pmin) && box1.contains(box2.pmax);
}

inline bool intersects(ray& r, float3 const& invrd, bbox const& box)
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
