#ifndef AOTRACER_H
#define AOTRACER_H

#include "tracer.h"
#include "../primitive/primitive.h"

class Sampler;

///< AoTracer is an implementation of a Tracer interface
///< capable of estimating ambient occlusion of a given radius
///<
class AoTracer : public Tracer
{
public:
    AoTracer(float radius)
    : radius_(radius)
    {
    }
    
    // Estimate a radiance coming from r
    float3 Li(ray& r, World const& world, Sampler const& lightsampler) const;
    
private:
    // Occlusion radius
    float radius_;
};

#endif // AOTRACER_H
