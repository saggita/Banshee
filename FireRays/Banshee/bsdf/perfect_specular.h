#ifndef PERFECT_SPECULAR_H
#define PERFECT_SPECULAR_H

#include "../math/mathutils.h"

#include "bsdf.h"
///< Specular reflection is the mirror-like reflection of light (or of other kinds of wave) from a surface, 
///< in which light from a single incoming direction (a ray) is reflected into a single outgoing direction. 
///< Such behavior is described by the law of reflection, which states that the direction of incoming light (the incident ray), 
///< and the direction of outgoing light reflected (the reflected ray) make the same angle with respect to the surface normal, 
///< thus the angle of incidence equals the angle of reflection (\theta _i = \theta _r in the figure), 
///< and that the incident, normal, and reflected directions are coplanar.
///< (c) Wikipedia
///<
class PerfectSpecular : public Bsdf
{
public:
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float3 const& wi, float3& wo, float& pdf) const
    {
        wo = normalize(2.f * dot(isect.n, wi) * isect.n - wi);
        return float3(1.f, 1.f, 1.f);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        float3 reflected = normalize(2.f * dot(isect.n, wi) * isect.n - wi);
        float  delta = (reflected - wo).sqnorm(); 
        return fabsf(delta) < 0.001f ? float3(1.f, 1.f, 1.f) : float3(0.f, 0.f, 0.f);
    }
};


#endif // PERFECT_SPECULAR_H