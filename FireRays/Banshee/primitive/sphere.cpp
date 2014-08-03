#include "sphere.h"

#include "../math/mathutils.h"

bool Sphere::Intersect(ray& r,  float& t, Intersection& isect) const
{
    // Transform ray into object space
    ray ro = transform_ray(r, worldmatinv_);

    // Calc equation coefs
    // (o.x + t * d.x)^2 + (o.y + t * d.y)^2 + (o.z + t * d.z)^2 = r^2
    float a = ro.d.x * ro.d.x + ro.d.y * ro.d.y + ro.d.z * ro.d.z;
    float b = 2 * (ro.o.x * ro.d.x + ro.o.y * ro.d.y + ro.o.z * ro.d.z);
    float c = ro.o.x * ro.o.x + ro.o.y * ro.o.y + ro.o.z * ro.o.z - radius_ * radius_;

    float t0, t1;
    if (solve_quadratic(a, b, c, t0, t1))
    {
        if (t0 > ro.t.y || t1 < ro.t.x)
            return false;

        float tt = t0;

        if (tt > ro.t.x)
        {
            t = tt;
            FillIntersectionInfo(ro(t), isect);
        }
        else
        {
            tt = t1;

            if (tt < ro.t.y)
            {
                t = tt;
                FillIntersectionInfo(ro(t), isect);
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool Sphere::Intersect(ray& r) const
{
    // Transfrom ray into object space
    ray ro = transform_ray(r, worldmatinv_);

    // Calc equation coefs
    // (o.x + t * d.x)^2 + (o.y + t * d.y)^2 + (o.z + t * d.z)^2 = r^2
    float a = ro.d.x * ro.d.x + ro.d.y * ro.d.y + ro.d.z * ro.d.z;
    float b = 2 * (ro.o.x * ro.d.x + ro.o.y * ro.d.y + ro.o.z * ro.d.z);
    float c = ro.o.x * ro.o.x + ro.o.y * ro.o.y + ro.o.z * ro.o.z - radius_ * radius_;

    float t0, t1;
    if (solve_quadratic(a, b, c, t0, t1))
    {
        if (t0 > ro.t.y || t1 < ro.t.x)
            return false;

        float tt = t0;

        if (tt > ro.t.x)
        {
            return true;
        }
        else
        {
            tt = t1;

            if (tt < ro.t.y)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
}

bbox Sphere::Bounds() const
{
    float3 pmin(-radius_, -radius_, -radius_);
    float3 pmax( radius_,  radius_,  radius_);
    return bbox(transform_point(pmin, worldmat_), transform_point(pmax, worldmat_));
}

void Sphere::FillIntersectionInfo(float3 const& p, Intersection& isect) const
{
    float3 n = normalize(p);

    isect.p = transform_point(p, worldmat_);

    // TODO: need to optimize this
    isect.n = transform_normal(n, worldmat_);

    // TODO: not supported atm
    isect.uv = float2(0,0);
    isect.m  = 0;
}