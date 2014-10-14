#ifndef MATTE_H
#define MATTE_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/lambert.h"

///< Matte material is providing the look of a diffuse surfaces
///<
class Matte : public Material
{
public:
    // If diffuse map is specified it is used as a diffuse color, otherwise diffuse color is used
    Matte (TextureSystem const& texturesys, 
        float3 const& diffuse, 
        std::string const& diffusemap = "",
        std::string const& normalmap = "")
        : Material(texturesys)
        , diffuse_(diffuse)
        , diffusemap_(diffusemap)
        , normalmap_(normalmap)
        , bsdf_(new Lambert())
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Copy to be able to alter normal
        Primitive::Intersection isectlocal = isect;

        if (!normalmap_.empty())
        {
            MapNormal(normalmap_, isectlocal);
        }

        // TODO: add support for ray differentials
        float  ndotwi = std::max(dot(isectlocal.n, wi), 0.f);
        float3 kd = diffusemap_.empty() ? diffuse_ : texturesys_.Sample(diffusemap_, isect.uv, float2(0,0));
        return kd * bsdf_->Sample(isectlocal, sample, wi, wo, pdf) * ndotwi;
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Copy to be able to alter normal
        Primitive::Intersection isectlocal = isect;

        if (!normalmap_.empty())
        {
            MapNormal(normalmap_, isectlocal);
        }

        // TODO: add support for ray differentials
        float  ndotwi = std::max(dot(isectlocal.n, wi), 0.f);
        float3 kd = diffusemap_.empty() ? diffuse_ : texturesys_.Sample(diffusemap_, isect.uv, float2(0,0));
        return kd * bsdf_->Evaluate(isectlocal, wi, wo) * ndotwi;
    }

    // Diffuse color
    float3 diffuse_;
    // Diffuse map
    std::string diffusemap_;
    // Normal map
    std::string normalmap_;
    // BSDF
    std::unique_ptr<Bsdf> bsdf_;
};

#endif // MATTE_H