#include "indexed_triangle.h"

bool IndexedTriangle::Intersect(ray& r, float& t, Intersection& isect) const
{
    float3 p1 = p_[pidx1_];
    float3 p2 = p_[pidx2_];
    float3 p3 = p_[pidx3_];

    float3 e1 = p2 - p1;
    float3 e2 = p3 - p1;

    float3 s1 = cross(r.d, e2);
    float  invd = 1.f/dot(s1, e1);

    float3 d = r.o - p1;
    float  b1 = dot(d, s1) * invd;

    if (b1 < 0.f || b1 > 1.0)
        return false;

    float3 s2 = cross(d, e1);
    float  b2 = dot(r.d, s2) * invd;

    if (b2 < 0.f || b1 + b2 > 1.f)
        return false;

    float tmp = dot(e2, s2) * invd;

    if (tmp > r.t.x && tmp <= r.t.y)
    {
        t = tmp;

        float3 n1 = n_[nidx1_];
        float3 n2 = n_[nidx2_];
        float3 n3 = n_[nidx3_];

        float2 t1 = t_[tidx1_];
        float2 t2 = t_[tidx2_];
        float2 t3 = t_[tidx3_];

        isect.p = (1.f - b1 - b2) * p1 + b1 * p2 + b2 * p3;
        isect.n = (1.f - b1 - b2) * n1 + b1 * n2 + b2 * n3;
        isect.t = (1.f - b1 - b2) * t1 + b1 * t2 + b2 * t3;
        isect.m = m_;

        return true;
    }

    return false;
}

bool IndexedTriangle::Intersect(ray& r) const
{
    float3 p1 = p_[pidx1_];
    float3 p2 = p_[pidx2_];
    float3 p3 = p_[pidx3_];

    float3 e1 = p2 - p1;
    float3 e2 = p3 - p1;

    float3 s1 = cross(r.d, e2);
    float  invd = 1.f/dot(s1, e1);

    float3 d = r.o - p1;
    float  b1 = dot(d, s1) * invd;

    if (b1 < 0.f || b1 > 1.0)
        return false;

    float3 s2 = cross(d, e1);
    float  b2 = dot(r.d, s2) * invd;

    if (b2 < 0.f || b1 + b2 > 1.f)
        return false;

    float tmp = dot(e2, s2) * invd;

    if (tmp > r.t.x && tmp <= r.t.y)
    {
        t = tmp;
        return true;
    }

    return false;
}

bbox IndexedTriangle::Bounds() const
{
    bbox box(p_[pidx1_], p_[pidx2_]);
    return bboxunion(box, p_[pidx3_]);
}