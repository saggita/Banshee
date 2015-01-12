#include "gitracer.h"

#include "../world/world.h"
#include "../sampler/sampler.h"

float3 GiTracer::Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler, bool countemissives) const
{
    Primitive::Intersection isect;
    float t = r.t.y;
    float3 radiance;

    if (world.Intersect(r, t, isect))
    {
        // If we hit emissive object just return its emission characteristic
        Material const& mat = *world.materials_[isect.m];

        if (mat.emissive())
        {
            return countemissives ? mat.Le(Primitive::SampleData(isect), -r.d) : 0.f;
        }
        else
        {
            // Evaluate DI component
            for (int i = 0; i < world.lights_.size(); ++i)
            {
                radiance += Di(world, *world.lights_[i], lightsampler, -r.d, isect);
            }

            // Evaluate indirect
            if (r.id < maxdepth_ && indirect_contrib_ > 0.f)
            {
                // Get the material for this intersection
                Material& m = *world.materials_[isect.m];
                // Generate new ray by sampling BSDF

                // BSDF sampling
                float3 indirect;
                int numsamples = brdfsampler.num_samples();
                std::vector<float2> samples(numsamples);

                for (int i = 0; i < numsamples; ++i)
                    samples[i] = brdfsampler.Sample2D();

                ray newray;
                newray.o = isect.p;

                for (int i = 0; i < numsamples; ++i)
                {
                    float pdf;
                    newray.t = float2(0.002f, 10000.f);
                    newray.id = r.id + 1;

                    float3 bsdf = m.Sample(isect, samples[i], -r.d, newray.d, pdf);

                    float3 le = Li(newray, world, lightsampler, brdfsampler, false);

                    // TODO: move PDF treshold to global settings
                    // TODO: accound for quadratic falloff
                    if (bsdf.sqnorm() > 0.f && le.sqnorm() > 0.f && pdf > 0.05f)
                    {
                        indirect += indirect_contrib_ * bsdf * le * (1.f / pdf);
                    }
                }

                indirect *= (1.f/numsamples);
                radiance += indirect;
            }
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