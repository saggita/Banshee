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

inline bool intersects(ray& r, bbox const& box)
{
    float3 rd = float3(1.f / r.d.x, 1.f / r.d.y, 1.f / r.d.z);
    float lo = rd.x * (box.pmin.x - r.o.x);
    float hi = rd.x * (box.pmax.x - r.o.x);

    float tmin = std::min(lo, hi);
    float tmax = std::max(lo, hi);

    float lo1 = rd.y*(box.pmin.y - r.o.y);
    float hi1 = rd.y*(box.pmax.y - r.o.y);

    tmin = std::max(tmin, std::min(lo1, hi1));
    tmax = std::min(tmax, std::max(lo1, hi1));

    float lo2 = rd.z*(box.pmin.z - r.o.z);
    float hi2 = rd.z*(box.pmax.z - r.o.z);

    tmin = std::max(tmin, std::min(lo2, hi2));
    tmax = std::min(tmax, std::max(lo2, hi2));

    if ((tmin <= tmax) && (tmax > r.t.x))
    {
        return (tmin >= r.t.x) ? (tmin < r.t.y) : (tmax < r.t.y);
    }
    else
        return false;
}

#endif // BBOX_H
