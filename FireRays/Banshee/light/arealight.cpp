#include "arealight.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include "../primitive/primitive.h"
#include "../material/material.h"

AreaLight::AreaLight(Primitive const& primitive, Material const& material)
    : primitive_(primitive)
    , material_(material)
{
    // TODO: dont like this code. Design issue?
    if(primitive_.surface_area() <= 0.f)
    {
        throw std::runtime_error("AreaLight: The primitive passed cannot be used as area light");
    }

    if (!material_.emissive())
    {
        throw std::runtime_error("AreaLight: The material passed is not an emissive one");
    }
}


float3 AreaLight::Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const
{
    // Get the sample point in world space
    Primitive::SampleData sampledata;
    primitive_.Sample(sample, sampledata, pdf);

    if (pdf > 0.f)
    {
        // Set direction to the light
        d = sampledata.p - isect.p;

        // Emissive power with squared fallof
        float d2inv = 1.f / d.sqnorm();

        // Return emission characteristic of the material
        return material_.Le(sampledata, -normalize(d)) * d2inv;
    }

    return float3(0,0,0);
}