#include "aotracer.h"

#include "../world/world.h"
#include "../sampler/sampler.h"
#include "../math/mathutils.h"

#include <algorithm>

float3 AoTracer::Li(ray& r, World const& world, Sampler const& lightsampler) const
{
    Primitive::Intersection isect;
    float t = r.t.y;
    float3 visibility = float3(1.f, 1.f, 1.f);
    
    if (world.Intersect(r, t, isect))
    {
        int numsamples = lightsampler.num_samples();
        float3 occlusion;
        
        for (int i=0; i<numsamples; ++i)
        {
            float2 sample = lightsampler.Sample2D();
            
            ray r;
            r.d = map_to_hemisphere(isect.n, sample, 2.f);
            r.o = isect.p;
            r.t = float2(0.0002f, radius_);
            
            if (world.Intersect(r))
            {
                occlusion += float3(1.f, 1.f, 1.f);
            }
        }
        
        occlusion *= (1.f/numsamples);
        visibility = float3(1.f, 1.f, 1.f) - occlusion;
        
    }
    
    return visibility;
}
