#ifndef TRACER_H
#define TRACER_H

class World;
class Sampler;

#include "../math/ray.h"

///< Tracer is an entity estimating a radiance along a given ray
///<
class Tracer
{
public:
    Tracer(){}
    // Destructor
    virtual ~Tracer(){}

    // Estimate a radiance coming from r
    virtual float3 Li(ray& r, World const& world, Sampler const& sampler) const = 0;

protected:
    Tracer(Tracer const&);
    Tracer& operator = (Tracer const&);
};


#endif // TRACER_H