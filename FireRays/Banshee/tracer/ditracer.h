#ifndef DITRACER_H
#define DITRACER_H

#include "tracer.h"
#include "../primitive/primitive.h"

class Light;

///< DiTracer is an implementation of a Tracer interface
///< capable of estimating direct illumination from surfaces
///<
class DiTracer : public Tracer
{
public:
    DiTracer(){}

    // Estimate a radiance coming from r
    float3 Li(ray& r, World const& world) const;

protected:
    virtual float3 Shade(World const& world, Light const& light, Primitive::Intersection& isect) const;
};

#endif