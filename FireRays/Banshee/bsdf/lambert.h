#ifndef LAMBERT_H
#define LAMBERT_H

#include "../math/mathutils.h"

#include "bsdf.h"
///< Lambertian reflectance is the property that defines an ideal "matte" or diffusely reflecting surface. 
///< The apparent brightness of such a surface to an observer is the same regardless of the observer's angle of view. 
///< More technically, the surface's luminance is isotropic, and the luminous intensity obeys Lambert's cosine law. 
///< Lambertian reflectance is named after Johann Heinrich Lambert, who introduced the concept of perfect diffusion in his 1760 book Photometria.
///< (c) Wikipedia
///<
class Lambert : public Bsdf
{
public:
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float3 const& wi, float3& wo, float& pdf) const
    {
        // TODO: fake for now, generate 2 numbers and choose random direction
        // will need to account for samling strategy later and provide a sampler
        float invpi = 1.f / PI;
        float2 sample(rand_float(), rand_float());
        wo = map_to_hemisphere(isect.n, sample, 1.f);
        pdf = 1;
        return float3(invpi, invpi, invpi);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        float invpi = 1.f / PI;
        return float3(invpi, invpi, invpi);
    }
};


#endif // LAMBERT_H