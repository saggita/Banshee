#include "gitracer.h"

#include "../world/world.h"

float3 GiTracer::Li(ray& r, World const& world, Sampler const& lightsampler) const
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
            ray newray;
            newray.o = isect.p;
            newray.t = float2(0.002f, 10000.f);
            newray.id = r.id + 1;
            float pdf = 1.f;
            
            // BSDF
            float3 bsdf = m.Sample(isect, -r.d, newray.d, pdf);
            
            // Indirect incoming light
            float3 le = Li(newray, world, lightsampler);
            
            // ndotwi factor
            float ndotwi = dot(newray.d, isect.n);
            
            if (ndotwi > 0.f && bsdf.sqnorm() > 0.f && le.sqnorm() > 0.f)
            {
                radiance += indirect_contrib_ * bsdf * le * ndotwi;
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