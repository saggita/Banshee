#include "gitracer.h"

#include <cassert>

#include "../world/world.h"
#include "../sampler/sampler.h"
#include "../bsdf/bsdf.h"
#include "../math/mathutils.h"

float3 GiTracer::Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    // Accumulated radiance
    float3 radiance = float3();
    
    // Intersection along the ray
    Primitive::Intersection isectprimary;
    
    // Keep ray distance around
    float t = r.t.y;
    
    // Check primary ray
    if (!world.Intersect(r, t, isectprimary))
    {
        radiance += world.bgcolor_;
        
        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += world.lights_[i]->Le(r);
        }
        
        // Bail out as the ray missed geometry
        return radiance;
    }
    
    // Allocate BRDF samples
    int numsamples = brdfsampler.num_samples();
    std::vector<float2> samples(numsamples);
    for (int s=0; s<numsamples; ++s)
    {
        samples[s] = brdfsampler.Sample2D();
    }
    
    
    // Start calculating indrect paths
    for (int s=0;s<numsamples;++s)
    {
        // Temporary ray - restore primary hit point
        ray rr = r;
        
        // Isect
        Primitive::Intersection isect = isectprimary;
        
        // Path throughput
        float3 throughput = float3(1.f, 1.f, 1.f);
        
        // Start calculating bounces
        for (int bounce=0; bounce<maxdepth_; ++bounce)
        {
            Material const& mat = *world.materials_[isect.m];
            
            // If we hit emissive object as a first bounce add its contribution
            if (bounce == 0 && mat.emissive())
            {
                Primitive::SampleData sampledata(isect);
                radiance += mat.Le(sampledata, -rr.d);
                
                // TODO: implement mixed emissive-bsdf materials
                break;
            }
            
            // Evaluate DI component
            for (int i = 0; i < world.lights_.size(); ++i)
            {
                radiance += throughput * Di(world, *world.lights_[i], lightsampler, brdfsampler, -rr.d, isect);
            }
            
            
            // Sample BSDF to continue path
            float2 bsdfsample = samples[s];
            // BSDF type
            int bsdftype = 0;
            // BSDF PDF
            float bsdfpdf = 0.f;
            // Direction
            float3 wi;
            
            // Sample material
            float3 bsdf = mat.Sample(isect, bsdfsample, -rr.d, wi, bsdfpdf, bsdftype);
            
            // Bail out if zero BSDF sampled
            if (bsdf.sqnorm() == 0.f || bsdfpdf == 0.f)
            {
                break;
            }
            
            // Construct ray
            rr.o = isect.p;
            rr.d = normalize(wi);
            rr.t = float2(0.001f, 1000000.f);
            
            // Update througput
            throughput *= (bsdf * (fabs(dot(isect.n, wi)) / bsdfpdf));
            
            //assert(!has_nans(throughput));
            
            // Apply Russian roulette
            // TODO: pass RNG inside not to have this ugly dep
            if (bounce > 3)
            {
                float rnd = rand_float();
                
                float luminance = 0.2126f * throughput.x + 0.7152f * throughput.y + 0.0722f * throughput.z;
                
                float q = std::min(0.5f, luminance);
                
                if (rnd > q)
                    break;
                
                throughput *= (1.f / q);
            }
            
            // Ray distance
            t = rr.t.y;
            
            if (!world.Intersect(rr, t, isect))
            {
                break;
            }
            
        }
    }
    
    return radiance  * (1.f / numsamples);
}