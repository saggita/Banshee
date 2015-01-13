#ifndef PERFECT_REFRACT_H
#define PERFECT_REFRACT_H

#include "../math/mathutils.h"

#include "bsdf.h"
///< In optics, refraction is a phenomenon that often occurs when waves travel 
///< from a medium with a given refractive index to a medium with another at an oblique angle.
///< At the boundary between the media, the wave's phase velocity is altered, usually causing a change in direction. 
///< (c) Wikipedia
///<
class PerfectRefract : public Bsdf
{
public:

    // Constructor
    PerfectRefract(float eta)
        : eta_(eta)
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        float3 n;
        float eta;
        float ndotwi = dot(wi, isect.n);

        if (ndotwi >= 0.f)
        {
            n = isect.n;
            eta = eta_;
        }
        else
        {
            n = -isect.n;
            eta = 1 / eta_;
            ndotwi = -ndotwi;
        }

        float q = 1.f - (1 - ndotwi * ndotwi) / (eta*eta);

        if (q >= 0)
        {
            wo = - (1.f / eta) * wi - (sqrtf(q) - ndotwi / eta)*n;
            pdf = 1.f;
            return float3(1.f, 1.f, 1.f);
        }
        else
        {
            pdf = 0.f;
            return float3(0.f, 0.f, 0.f);
        }
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {


        return float3(0.f, 0.f, 0.f);
    }

private:
    // Index of refraction
    float eta_;
};


#endif // PERFECT_REFRACT_H