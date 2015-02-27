#include "environment_light.h"

#include "../texture/texturesystem.h"
#include "../math/mathutils.h"

#include <cassert>

float3 EnvironmentLight::Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const
{
    // Precompute invpi
    float invpi = 1.f / PI;

    // Generate random hemispherical direction with cosine distribution
    d = map_to_hemisphere(isect.n, sample, 1.f);

    // PDF is proportional to dot(n,d) since the distribution
    // and normalized
    pdf = dot(isect.n, d) * invpi;

    // Convert world d coordinates to spherical representation
    float r, phi, theta;
    cartesian_to_spherical(d, r, phi, theta);

    // Compose the textcoord to fetch
    float2 uv(phi / (2*PI), theta / PI);

    // Fetch radiance value and scale it
    return scale_ * texsys_.Sample(texture_, uv, float2(0,0));
}


float3 EnvironmentLight::Le(ray const& r) const
{
     // Convert world d coordinates to spherical representation
    float rr, phi, theta;
    cartesian_to_spherical(r.d, rr, phi, theta);

    // Compose the textcoord to fetch
    float2 uv(phi / (2*PI), theta / PI);

    // Fetch radiance value and scale it
    return scale_ * texsys_.Sample(texture_, uv, float2(0,0));
}

// PDF of a given direction sampled from isect.p
float EnvironmentLight::Pdf(Primitive::Intersection const& isect, float3 const& w) const
{
    // Precompute invpi
    float invpi = 1.f / PI;
    
    // PDF is proportional to dot(n,d)
    return dot(isect.n, normalize(w)) * invpi;
}