#include "arealight.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include "../primitive/shapebundle.h"
#include "../material/material.h"
#include "../math/mathutils.h"

AreaLight::AreaLight(size_t shapeidx, ShapeBundle& bundle, Material const& material)
    : bundle_(bundle)
    , material_(material)
    , shapeidx_(shapeidx)
{
    if (!material_.IsEmissive())
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
    
    bundle_.GetSampleOnShape(shapeidx_, sample, sampledata);
    
    // Set direction to the light
    d = sampledata.p - hit.p;
    
    // If the object facing the light compute emission
    if (sampledata.pdf > 0.f && dot(sampledata.n, -normalize(d)) > 0.f)
    {
        // Return PDF
        pdf = sampledata.pdf;
        
        // Emissive power with squared fallof
        float d2inv = 1.f / d.sqnorm();

        // Return emission characteristic of the material
        return material_.GetLe(sampledata, -normalize(d)) * d2inv;
    }

    
    // Otherwise just set probability to 0
    pdf = 0.f;
    return float3();
}

float AreaLight::GetPdf(ShapeBundle::Hit const& hit, float3 const& w) const
{
    return bundle_.GetPdfOnShape(shapeidx_, hit.p, w);
}