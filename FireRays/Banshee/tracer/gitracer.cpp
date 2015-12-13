#include "gitracer.h"

#include <cassert>

#include "../world/world.h"
#include "../sampler/sampler.h"
#include "../bsdf/bsdf.h"
#include "../math/mathutils.h"

#define MINPDF 0.05f
#define MAXRADIANCE 4.f

float3 GiTracer::GetLi(ray const& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    // Accumulated radiance
    float3 radiance = float3();
    
    // Intersection along the ray
    ShapeBundle::Hit hitprimary;
    
    // Check primary ray
    if (!world.Intersect(r, hitprimary))
    {
        radiance += world.bgcolor_;
        
        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += world.lights_[i]->GetLe(r);
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
        
        // hit
        ShapeBundle::Hit hit = hitprimary;
        
        // Path throughput
        float3 throughput = float3(1.f, 1.f, 1.f);
        
        // Start calculating bounces
        for (int bounce=0; bounce<maxdepth_; ++bounce)
        {
            Material const& mat = *world.materials_[hit.m];
            
            // If we hit emissive object as a first bounce add its contribution
            if (bounce == 0 && mat.IsEmissive())
            {
                ShapeBundle::Sample sampledata(hit);
                radiance += mat.GetLe(sampledata, -rr.d);
                
                // TODO: implement mixed emissive-bsdf materials
                break;
            }
            
            // Evaluate DI component
            //
            if (bounce != 0)
            {
                int numlights = world.lights_.size();
                int idx = rand_uint() % numlights;
                radiance += throughput * GetDi(world, *world.lights_[idx], lightsampler, brdfsampler, -rr.d, hit) * numlights;
            }
            else
            {
                for (int i = 0; i < world.lights_.size(); ++i)
                {
                    radiance += throughput * GetDi(world, *world.lights_[i], lightsampler, brdfsampler, -rr.d, hit);
                }
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
            float3 bsdf = mat.Sample(hit, bsdfsample, -rr.d, wi, bsdfpdf, bsdftype);
            
            // Bail out if zero BSDF sampled
            if (bsdf.sqnorm() == 0.f || bsdfpdf < MINPDF)
            {
                break;
            }
            
            // Construct ray
            rr.o = hit.p;
            rr.d = normalize(wi);
            rr.t = float2(0.01f, 1000000.f);
            
            // Update througput
            throughput *= (bsdf * (fabs(dot(hit.n, wi)) / bsdfpdf));
            
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
            
            if (!world.Intersect(rr, hit))
            {
                break;
            }
            
        }
    }
    
    return clamp(radiance * (1.f / numsamples), float3(0.f, 0.f, 0.f), float3(MAXRADIANCE, MAXRADIANCE, MAXRADIANCE));
}