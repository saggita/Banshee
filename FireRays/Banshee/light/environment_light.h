#ifndef ENVIRONMENT_LIGHT_H
#define ENVIRONMENT_LIGHT_H

class TextureSystem;

#include "light.h"

///< EnvironmentLight represents image based light emitting from the whole sphere around.
///< The radiance values are taken from HDR lightprobe provided during construction.
///<
class EnvironmentLight: public Light
{
public:
    EnvironmentLight(TextureSystem const& texsys,
                     std::string const& texture,
                     float scale = 1.f)
                     : texsys_(texsys)
                     , texture_(texture)
                     , scale_(scale)
    {
    }

    // Sample method generates a direction to the light(d), calculates pdf in that direction (pdf)
    // and returns radiance emitted(return value) into the direction specified by isect
    // Note that no shadow testing occurs here, the method knows nothing about world's geometry
    // and it is renderers responsibility to account for visibility term
    // [PARTIALLY DONE] TODO: account for different sampling modes here, need to pass sampler/sample?
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const;
    
    // This method is supposed to be called by the renderer when the ray misses the geometry.
    // It allows implementing IBL, etc.
    float3 Le(ray const& r) const;

private:
    // Texture system
    TextureSystem const& texsys_;
    // Texture file name
    std::string texture_;
    // Radiance scale
    float scale_;
};

#endif // ENVIRONMENT_LIGHT_H