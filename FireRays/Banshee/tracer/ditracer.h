#ifndef DITRACER_H
#define DITRACER_H

#include "tracer.h"
#include "../primitive/primitive.h"

class Light;
class Sampler;

///< DiTracer is an implementation of a Tracer interface
///< capable of estimating direct illumination from surfaces
///<
class DiTracer : public Tracer
{
public:
    DiTracer(){}

    // Estimate a radiance coming from r
    float3 Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const;

protected:
    virtual float3 Di(World const& world, Light const& light, Sampler const& sampler, float3 const& wo, Primitive::Intersection& isect) const;
};

#endif // DITRACER_H