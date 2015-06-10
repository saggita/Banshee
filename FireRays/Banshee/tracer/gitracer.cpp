#include "gitracer.h"

#include <cassert>

#include "../world/world.h"
#include "../sampler/sampler.h"
#include "../bsdf/bsdf.h"
#include "../math/mathutils.h"

float3 GiTracer::Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    // Intersection along the ray
    Primitive::Intersection isect;

    // Ray distance
    float t = r.t.y;

    // Accumulated radiance
    float3 radiance = float3();

    // Path throughput
    float3 throughput = float3(1.f, 1.f, 1.f);

    for (int bounce=0; bounce<maxdepth_; ++ bounce)
    {
        // If camera ray intersection occurs
        if (world.Intersect(r, t, isect))
        {
            Material const& mat = *world.materials_[isect.m];
            
            // If we hit emissive object as a first bounce add its contribution
            if (bounce == 0 && mat.emissive())
            {
                Primitive::SampleData sampledata(isect);
                radiance += mat.Le(sampledata, -r.d);
                
                // TODO: implement mixed emissive-bsdf materials
                break;
            }

            // Evaluate DI component
            for (int i = 0; i < world.lights_.size(); ++i)
            {
                radiance += throughput * Di(world, *world.lights_[i], lightsampler, brdfsampler, -r.d, isect);
            }

            //assert(!has_nans(radiance));

            // TODO: workaround for more than 1 brdf sample
            std::vector<float2> samples(brdfsampler.num_samples());
            for (int s=0; s<brdfsampler.num_samples(); ++s)
                samples[s] = brdfsampler.Sample2D();

            // Sample BSDF to continue path
            float2 bsdfsample = samples[rand_uint() % samples.size()];
            // BSDF type
            int bsdftype = 0;
            // BSDF PDF
            float bsdfpdf = 0.f;
            // Direction
            float3 wi;
            
            // Sample material
            float3 bsdf = mat.Sample(isect, bsdfsample, -r.d, wi, bsdfpdf, bsdftype);
            
            // Bail out if zero BSDF sampled
            if (bsdf.sqnorm() == 0.f || bsdfpdf == 0.f)
            {
                break;
            }
            
            // Construct ray
            r.o = isect.p;
            r.d = normalize(wi);
            r.t = float2(0.001f, 10000.f);

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

            //assert(!has_nans(radiance));
        }
        else if (bounce == 0)
        {
            radiance += throughput * world.bgcolor_;

            for (int i = 0; i < world.lights_.size(); ++i)
            {
                radiance += throughput * world.lights_[i]->Le(r);
            }

            // Bail out as the ray missed geometry
            break;
        }
    }
    
    return radiance;
}