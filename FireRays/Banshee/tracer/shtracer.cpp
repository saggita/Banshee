#include "shtracer.h"
#include "../world/world.h"
#include "../math/sh.h"
#include "../math/shproject.h"

#define _USE_MATH_DEFINES
#include <math.h>

ShTracer::ShTracer(int lmax, float3* const shcoeffs)
: lmax_(lmax)
, shcoeffs_(lmax)
{
    shcoeffs_.resize(NumShTerms(lmax));
    for (int i=0; i<NumShTerms(lmax); ++i)
    {
        shcoeffs_[i] = shcoeffs[i];
    }
}

float3 ShTracer::GetE(float3 const& n) const
{
    std::vector<float3> convolvedcoeffs(NumShTerms(lmax_));
    std::vector<float>  ylm(NumShTerms(lmax_));
    
    ShConvolveCosTheta(lmax_, &shcoeffs_[0], &convolvedcoeffs[0]);
    
    ShEvaluate(n, lmax_, &ylm[0]);
    
    float3 radiance;
    for (int i=0; i<NumShTerms(lmax_); ++i)
    {
        radiance += ylm[i] * convolvedcoeffs[i] * (0.7f / M_PI);
    }
    
    return radiance;
}


float3 ShTracer::GetLi(ray const& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    ShapeBundle::Hit hit;
    float3 radiance;
    
    if (world.Intersect(r, hit))
    {
        radiance = GetE(dot(-r.d, hit.n) > 0 ? hit.n : -hit.n);
    }
    else
    {
        radiance = world.bgcolor_;
        
        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += world.lights_[i]->GetLe(r);
        }
    }
    
    return radiance;
}