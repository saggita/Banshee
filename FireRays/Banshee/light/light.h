#ifndef LIGHT_H
#define LIGHT_H

#include "../primitive/primitive.h"
#include "../math/float3.h"

///< Light serves as an interface to all light types
///< supported by the system.
///<
class Light
{
public:
    virtual ~Light(){}
    
    // Sample method generates a point on the light(p), calculates pdf in that point (pdf)
    // and returns radiance emitted(return value) into the direction specified by isect
    // Note that no shadow testing occurs here, the method knows nothing about world's geometry
    // and it is renderers responsibility to account for visibility term
    // [PARTIALLY DONE] TODO: account for different sampling modes here, need to pass sampler/sample?
    virtual float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3& p, float& pdf) const = 0;
    
    // This method is supposed to be called by the renderer when the ray misses the geometry.
    // It allows implementing IBL, etc.
    virtual float3 Le(ray const& r) const = 0;
};

#endif // LIGHT_H