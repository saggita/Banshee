#include "gitracer.h"

#include "../world/world.h"
#include "../sampler/sampler.h"

float3 GiTracer::Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    Primitive::Intersection isect;
    float t = r.t.y;
    float3 radiance;

    if (world.Intersect(r, t, isect))
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
                float pdf = 1.f;
                newray.t = float2(0.002f, 10000.f);
                newray.id = r.id + 1;

                float3 bsdf = m.Sample(isect, samples[i], -r.d, newray.d, pdf);

                float3 le = Li(newray, world, lightsampler, brdfsampler);

                if (bsdf.sqnorm() > 0.f && le.sqnorm() > 0.f)
                {
                    indirect += indirect_contrib_ * bsdf * le;
                }
            }

            indirect *= (1.f/numsamples);
            radiance += indirect;
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