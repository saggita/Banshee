#include "indexed_triangle.h"
#include "mesh.h"

#include "../math/mathutils.h"

bool IndexedTriangle::Intersect(ray& r, float& t, Intersection& isect) const
{
    // Transform ray to local space using mesh world transform
    ray ro = transform_ray(r, mesh_.worldmatinv_);

    float3 p1 = mesh_.vertices_[pidx1_];
    float3 p2 = mesh_.vertices_[pidx2_];
    float3 p3 = mesh_.vertices_[pidx3_];

    float3 e1 = p2 - p1;
    float3 e2 = p3 - p1;

    float3 s1 = cross(ro.d, e2);
    float  invd = 1.f/dot(s1, e1);

    float3 d = ro.o - p1;
    float  b1 = dot(d, s1) * invd;

    if (b1 < 0.f || b1 > 1.0)
        return false;

    float3 s2 = cross(d, e1);
    float  b2 = dot(ro.d, s2) * invd;

    if (b2 < 0.f || b1 + b2 > 1.f)
        return false;

    float tmp = dot(e2, s2) * invd;

    if (tmp > ro.t.x && tmp <= ro.t.y)
    {
        t = tmp;

        float3 n1 = mesh_.normals_[nidx1_];
        float3 n2 = mesh_.normals_[nidx2_];
        float3 n3 = mesh_.normals_[nidx3_];

        float2 t1 = mesh_.uvs_[tidx1_];
        float2 t2 = mesh_.uvs_[tidx2_];
        float2 t3 = mesh_.uvs_[tidx3_];

        isect.p = transform_point((1.f - b1 - b2) * p1 + b1 * p2 + b2 * p3, mesh_.worldmat_);
        isect.n = normalize(transform_normal((1.f - b1 - b2) * n1 + b1 * n2 + b2 * n3, mesh_.worldmat_));
        // Account for backfacing normal
        if (dot(isect.n, -r.d) < 0.f) isect.n = -isect.n;

        float du1 = t1.x - t3.x;
        float du2 = t2.x - t3.x;
        float dv1 = t1.y - t3.y;
        float dv2 = t2.y - t3.y;

        float3 dp1 = p1 - p3;
        float3 dp2 = p2 - p3;

        float det = du1 * dv2 - dv1 * du2;

        if (det != 0.f)
        {
            float invdet = 1.f / det;
            isect.dpdu = normalize(( dv2 * dp1 - dv1 * dp2) * invdet);
            isect.dpdv = normalize((-du2 * dp1 + du1 * dp2) * invdet);
        }
        else
        {
            isect.dpdu = orthovector(isect.n);
            isect.dpdv = cross(isect.n, isect.dpdu);
        }

        isect.uv = (1.f - b1 - b2) * t1 + b1 * t2 + b2 * t3;
        isect.m = m_;

        return true;
    }

    return false;
}

bool IndexedTriangle::Intersect(ray& r) const
{
    ray ro = transform_ray(r, mesh_.worldmatinv_);

    float3 p1 = mesh_.vertices_[pidx1_];
    float3 p2 = mesh_.vertices_[pidx2_];
    float3 p3 = mesh_.vertices_[pidx3_];

    float3 e1 = p2 - p1;
    float3 e2 = p3 - p1;

    float3 s1 = cross(ro.d, e2);
    float  invd = 1.f/dot(s1, e1);

    float3 d = ro.o - p1;
    float  b1 = dot(d, s1) * invd;

    if (b1 < 0.f || b1 > 1.0)
        return false;

    float3 s2 = cross(d, e1);
    float  b2 = dot(ro.d, s2) * invd;

    if (b2 < 0.f || b1 + b2 > 1.f)
        return false;

    float tmp = dot(e2, s2) * invd;

    if (tmp > ro.t.x && tmp <= ro.t.y)
    {
        return true;
    }

    return false;
}

bbox IndexedTriangle::Bounds() const
{
    bbox box(mesh_.vertices_[pidx1_], mesh_.vertices_[pidx2_]);
    box = bboxunion(box, mesh_.vertices_[pidx3_]);
    return bbox(transform_point(box.pmin, mesh_.worldmat_), transform_point(box.pmax, mesh_.worldmat_));
}