#include "aotracer.h"

#include "../world/world.h"
#include "../sampler/sampler.h"
#include "../math/mathutils.h"

#include <algorithm>

float3 AoTracer::GetLi(ray const& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    ShapeBundle::Hit hit;
    // We need to return visibility here, white corresponds to unoccluded black to fully ocluded
    float3 visibility = float3(1.f, 1.f, 1.f);
    // If the ray hits the surface calculate the occlusion of the intersection point
    if (world.Intersect(r, hit))
    {
        // Number of times we need to sample occlusion
        //int numsamples = lightsampler.num_samples();
        //float3 occlusion;

        //for (int i=0; i<numsamples; ++i)
        //{
            // Get the sample
            //float2 sample = lightsampler.Sample2D();

            //ray r;
            // Map to hemisphere with empirically choosen cosine factor
            // TODO: move that into global settings
            // Prepare AO ray
            //r.d = map_to_hemisphere(hit.n, sample, 1.f);
            //r.o = hit.p;
            //r.t = float2(0.00002f, radius_);

            // Check an intersection
            //if (world.Intersect(r))
            //{
                //occlusion += float3(1.f, 1.f, 1.f);
            //}
        //}
        // Normalize occlusion factor
        //occlusion *= (1.f/numsamples);
        // Note that we need to return visibility, not occlusion
        //visibility = float3(1.f, 1.f, 1.f) - occlusion;
        return 0.5f * hit.n + float3(0.5f, 0.5f, 0.5f);
    }
    
    return float3();
}
