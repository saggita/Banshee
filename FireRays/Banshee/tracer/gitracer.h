#ifndef GITRACER_H
#define GITRACER_H

#include "ditracer.h"

///< GiTracer is an implementation of a Tracer interface
///< capable of estimating globall illumination from surfaces
///< up to specified number of bounces. It is not physically correct,
///< so uses user specified constant for indirect contribution weight.
///<
class GiTracer : public DiTracer
{
public:
    GiTracer(int maxdepth, float indirect_contrib)
        : maxdepth_(maxdepth)
        , indirect_contrib_(indirect_contrib)
    {}

    // Estimate a radiance coming from r
    float3 Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler, bool countemissives) const;

private:
    // Max depth to trace the ray to
    int maxdepth_;
    // Indirect contribution
    float indirect_contrib_;
};

#endif // GITRACER_H