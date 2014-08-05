#include "ditracer.h"

#include "../world/world.h"
#include "../light/light.h"

#include <algorithm>

float3 DiTracer::Li(ray& r, World const& world) const
{
    Primitive::Intersection isect;
    float t = r.t.y;
    float3 radiance;

    if (world.Intersect(r, t, isect))
    {
        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += Shade(isect, *world.lights_[i]);
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

float3 DiTracer::Shade(Primitive::Intersection& isect, Light const& light) const
{
    float  pdf; 
    float3 light_sample;
    float3 radiance = light.Sample(isect, light_sample, pdf);

    /// Simple diffuse shading for now
    float3 wi = normalize(light_sample - isect.p);

    return std::max(dot(wi, isect.n), 0.f) * radiance;
}