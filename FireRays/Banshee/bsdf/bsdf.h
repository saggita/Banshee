#ifndef BSDF_H
#define BSDF_H

#include "../primitive/primitive.h"

///< Bsdf is an abstraction for all BSDFs in the system
class Bsdf
{
public:
    // Destructor
    virtual ~Bsdf() {}

    // Sample material and return outgoing ray direction along with combined BSDF value
    virtual float3 Sample(Primitive::Intersection const& isect, float3 const& wi, float3& wo, float& pdf) const = 0;

    // Evaluate combined BSDF value
    virtual float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;
};


#endif // BSDF_H