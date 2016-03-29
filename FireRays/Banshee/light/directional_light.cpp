#include "directional_light.h"

#include <numeric>

float3 DirectionalLight::GetSample(ShapeBundle::Hit const& isect, float2 const& sample, float3& d, float& pdf) const
{
    // Need to return direction opposite to the light which is long enough
    // TODO: remove the constant
    d = -d_ * 100000000.f;
    // It's probability density == 1
    pdf = 1.f;
    // Emissive power with no falloff
    return e_;
}