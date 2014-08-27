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
        // Both roots are outside of the ray parameter range
        if (t0 > ro.t.y || t1 < ro.t.x)
            return false;

        // Take first candidate
        float tt = t0;

        if (tt > ro.t.x)
        {
            // First one is in the range
            t = tt;
            // Fill shading data
            FillIntersectionInfo(ro(t), isect);
        }
        else
        {
            // First one falls outside, take second
            tt = t1;

            if (tt < ro.t.y)
            {
                // Second one is in the range
                t = tt;
                // Fill shading data
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
    // Transform object space bbox into world space
    float3 pmin(-radius_, -radius_, -radius_);
    float3 pmax( radius_,  radius_,  radius_);
    return bbox(transform_point(pmin, worldmat_), transform_point(pmax, worldmat_));
}

void Sphere::FillIntersectionInfo(float3 const& p, Intersection& isect) const
{
    // Don't use cross(dpdu, dpdv) due to singularity
    // This normal is always fine
    float3 n = normalize(p);
    float r = sqrtf(p.sqnorm());

    // Calculate spherical coords
    float theta = acosf(p.z / r);
    float phi = atan2f(p.x, p.y);

    // Account for atan discontinuity
    if (phi < 0.f)
        phi += 2*PI;

    // Calculate partial derivatives
    float3 dpdu = float3(-2*PI*p.y, 2*PI*p.x, 0);
    float3 dpdv = float3(PI*p.z*cosf(phi), PI*p.z*sin(phi), -PI*r*sinf(theta));

    // Everything should be in world space in isect structure,
    // so transform position back into world space
    isect.p = transform_point(p, worldmat_);

    isect.n = normalize(transform_normal(n, worldmatinv_));

    // Remap spehrical coords into [0,1] uv range
    isect.uv = float2(phi/(2 * PI), theta/PI);

    // Transform partial derivatives into world space
    isect.dpdu = transform_vector(dpdu, worldmat_);
    isect.dpdv = transform_vector(dpdv, worldmat_);

    // TODO: support material system
    isect.m  = m_;
}