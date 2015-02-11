#ifndef DITRACER_H
#define DITRACER_H

#include "tracer.h"
#include "../primitive/primitive.h"

class Light;
class Sampler;

///< DiTracer is an implementation of a Tracer interface capable of estimating only direct illumination from surfaces. This implementation uses
///< multiple-importance sampling to reduce variance: https://graphics.stanford.edu/courses/cs348b-03/papers/veach-chapter9.pdf
///<
class DiTracer : public Tracer
{
public:
    // Constructor
    DiTracer(){}

    // Estimate a radiance coming from r due to direct illumination
    float3 Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler, bool countemissives) const;

protected:
    // Estimate direct illimination component due to light contribution reflected along wo
    virtual float3 Di(World const& world, Light const& light, Sampler const& lightsampler, Sampler const& bsdfsampler, float3 const& wo, Primitive::Intersection& isect) const;
};

#endif // DITRACER_H