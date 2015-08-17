#include "arealight.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include "../primitive/shapebundle.h"
#include "../material/material.h"

AreaLight::AreaLight(ShapeBundle& bundle, Material const& material)
    : bundle_(bundle)
    , material_(material)
{
    if (!material_.emissive())
    {
        throw std::runtime_error("AreaLight: The material passed is not an emissive one");
    }
    
    // Set this area light for the primitive
    bundle_.arealight_ = this;
}

float3 AreaLight::GetSample(ShapeBundle::Hit const& hit, float2 const& sample, float3& d, float& pdf) const
{
    // Get the sample point in world space
    ShapeBundle::Sample sampledata;
    primitive_.Sample(sample, sampledata, pdf);

    // Set direction to the light
    d = sampledata.p - isect.p;
    
    // If the object facing the light compute emission
    if (pdf > 0.f && dot(sampledata.n, -normalize(d)) > 0.f)
    {
        // Emissive power with squared fallof
        float d2inv = 1.f / d.sqnorm();

        // Return emission characteristic of the material
        return material_.Le(sampledata, -normalize(d)) * d2inv;
    }
    else
    {
        // Otherwise just set probability to 0
        pdf = 0.f;
        return float3(0,0,0);
    }
}

float AreaLight::GetPdf(Primitive::Intersection const& isect, float3 const& w) const
{
    return primitive_.Pdf(isect.p, w);
}