#include "arealight.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include "../primitive/primitive.h"
#include "../material/material.h"

AreaLight::AreaLight(Primitive& primitive, Material const& material)
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
    
    // Set this area light for the primitive
    primitive_.arealight_ = this;
}


float3 AreaLight::Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const
{
    // Get the sample point in world space
    Primitive::SampleData sampledata;
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

float AreaLight::Pdf(Primitive::Intersection const& isect, float3 const& w) const
{
    return primitive_.Pdf(isect.p, w);
}