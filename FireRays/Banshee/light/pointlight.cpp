#include "pointlight.h"

float3 PointLight::Sample(Primitive::Intersection const& isect, float3& p, float& pdf) const
{
    // Light position is the only possible sample point for a point light
    p = p_;
    // It's probability density == 1
    pdf = 1.f;
    // Emissive power
    return e_;
}