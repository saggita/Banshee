#include "sphere.h"

#include "..\math\mathutils.h"

bool Sphere::Intersect(ray& r,  float& t, Intersection& isect) const
{
    // Transform ray into object space
    ray or = transform_ray(r, worldmatinv_);

    // Calc equation coefs
    // (o.x + t * d.x)^2 + (o.y + t * d.y)^2 + (o.z + t * d.z)^2 = r^2
    float a = or.d.x * or.d.x + or.d.y * or.d.y + or.d.z * or.d.z;
    float b = 2 * (or.o.x * or.d.x + or.o.y * or.d.y + or.o.z * or.d.z);
    float c = or.o.x * or.o.x + or.o.y * or.o.y + or.o.z * or.o.z - radius_ * radius_;

    float t0, t1;
    if (solve_quadratic(a, b, c, t0, t1))
    {
        if (t0 > or.t.y || t1 < or.t.x)
            return false;

        float tt = t0;

        if (tt > or.t.x)
        {
            t = tt;
            FillIntersectionInfo(or(t), isect);
        }
        else
        {
            tt = t1;

            if (tt < or.t.y)
            {
                t = tt;
                FillIntersectionInfo(or(t), isect);
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
    // Transform ray into object space
    ray or = transform_ray(r, worldmatinv_);

    // Calc equation coefs
    // (o.x + t * d.x)^2 + (o.y + t * d.y)^2 + (o.z + t * d.z)^2 = r^2
    float a = or.d.x * or.d.x + or.d.y * or.d.y + or.d.z * or.d.z;
    float b = 2 * (or.o.x * or.d.x + or.o.y * or.d.y + or.o.z * or.d.z);
    float c = or.o.x * or.o.x + or.o.y * or.o.y + or.o.z * or.o.z - radius_ * radius_;

    float t0, t1;
    if (solve_quadratic(a, b, c, t0, t1))
    {
        if (t0 > or.t.y || t1 < or.t.x)
            return false;

        float tt = t0;

        if (tt > or.t.x)
        {
            return true;
        }
        else
        {
            tt = t1;

            if (tt < or.t.y)
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