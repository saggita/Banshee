#ifndef GITRACER_H
#define GITRACER_H

#include "ditracer.h"

///< GiTracer is an implementation of a Tracer interface
///< capable of estimating globall illumination from surfaces
///< up to specified number of bounces or termination due to Russian roulette criteria.
///< Provides physically correct unbiased estimate.
///<
class GiTracer : public DiTracer
{
public:
    // Constructor
    GiTracer(int maxdepth, float indirect_contrib)
        : maxdepth_(maxdepth)
    {}

    // Estimate a radiance coming from r
    float3 Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const;

private:
    // Max depth to trace the ray to
    int maxdepth_;
};

#endif // GITRACER_H