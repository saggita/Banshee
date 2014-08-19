#ifndef MATTE_H
#define MATTE_H

#include <string>

#include "material.h"
#include "../bsdf/lambert.h"

///< Matte material is providing the look of a diffuse surfaces
///<
class Matte : public Material
{
public:
    // If diffuse map is specified it is used as a diffuse color, otherwise diffuse color is used
    Matte (TextureSystem const& texturesys, float3 const& diffuse, std::string const& diffusemap = "")
        : Material(texturesys)
        , diffuse_(diffuse)
        , diffusemap_(diffusemap)
        , bsdf_(new Lambert())
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float3 const& wi, float3& wo, float& pdf) const
    {

        // TODO: add support for ray differentials
        float3 kd = diffusemap_.empty() ? diffuse_ : texturesys_.Sample(diffusemap_, isect.uv, float2(0,0));
        return kd * bsdf_->Sample(isect, wi, wo, pdf);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // TODO: add support for ray differentials
        float3 kd = diffusemap_.empty() ? diffuse_ : texturesys_.Sample(diffusemap_, isect.uv, float2(0,0));
        return kd * bsdf_->Evaluate(isect, wi, wo);
    }

    // Diffuse color
    float3 diffuse_;
    // Diffuse map
    std::string diffusemap_;
    // BSDF
    std::unique_ptr<Bsdf> bsdf_;
};

#endif // MATTE_H