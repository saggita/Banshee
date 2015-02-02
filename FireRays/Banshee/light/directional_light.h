#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H

#include "light.h"

///< The class represents the light which is infinitely far away.
///< This light has no distance drop-off. Might be used to emulate sunlight.
///<
class DirectionalLight: public Light
{
public:
    DirectionalLight(float3 const& d, float3 const& e)
    : d_(normalize(d))
    , e_(e)
    {}
    
    // Sample method generates a direction to the light(d), calculates pdf in that direction (pdf)
    // and returns radiance emitted(return value) into the direction specified by isect
    // Note that no shadow testing occurs here, the method knows nothing about world's geometry
    // and it is renderers responsibility to account for visibility term
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const;
    
    // This method is supposed to be called by the renderer when the ray misses the geometry.
    // It allows implementing IBL, etc.
    float3 Le(ray const& r) const
    {
        // Nothing should be emitted here for directional light
        return float3(0, 0, 0);
    }
    
    // PDF of a given direction sampled from isect.p
    float Pdf(Primitive::Intersection const& isect, float3 const& w) const
    {
        return 0.f;
    }
    
private:
    // World space direction
    float3 d_;
    // Emissive power
    float3 e_;
};

#endif // DIRECTIONALLIGHT_H
