#include "ditracer.h"

#include "../world/world.h"
#include "../light/light.h"
#include "../material/material.h"
#include "../sampler/sampler.h"

#include <algorithm>

float3 DiTracer::Li(ray& r, World const& world, Sampler const& lightsampler) const
{
    Primitive::Intersection isect;
    float t = r.t.y;
    float3 radiance;

    if (world.Intersect(r, t, isect))
    {
        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += Di(world, *world.lights_[i], lightsampler, -r.d, isect);
        }
    }
    else
    {
        radiance = world.bgcolor_;

        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += world.lights_[i]->Le(r);
        }
    }

    return radiance;
}

float3 DiTracer::Di(World const& world, Light const& light, Sampler const& sampler, float3 const& wo, Primitive::Intersection& isect) const
{
    float  pdf; 
    float3 light_sample;
    float3 radiance = float3(0,0,0);
    int numsamples = sampler.num_samples();
    
    for (int i=0; i<numsamples; ++i)
    {
        float2 sample = sampler.Sample2D();
        float3 le = light.Sample(isect, sample, light_sample, pdf);
        
        if (pdf > 0.f)
        {
            
            /// Simple diffuse shading for now
            float3 wi = normalize(light_sample - isect.p);
            float  dist = sqrtf((light_sample - isect.p).sqnorm());
            
            /// Spawn shadow ray
            ray shadowray;
            shadowray.o = isect.p;
            shadowray.d = wi;
            
            /// TODO: move ray epsilon into some global options object
            shadowray.t = float2(0.002f, dist);
            
            /// Check for an occlusion
            float shadow = world.Intersect(shadowray) ? 0.f : 1.f;

            if (shadow > 0.f && le.sqnorm() > 0.f)
            {
                Material const& mat = *world.materials_[isect.m];
                
                radiance +=  le * shadow * mat.Evaluate(isect, wi, wo);
            }
        }
    }

    return (1.f / numsamples) * radiance;
}