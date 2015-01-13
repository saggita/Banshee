#ifndef REFRACT_H
#define REFRACT_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/perfect_refract.h"

///< Refract material for glass like objects
///<
class Refract : public Material
{
public:
    // If color map is specified it is used as a refraction color, otherwise specified color value is used
    Refract (TextureSystem const& texturesys, 
        float eta,
        float3 const& color, 
        std::string const& colormap = "",
        std::string const& normalmap = "")
        : Material(texturesys)
        , color_(color)
        , colormap_(colormap)
        , normalmap_(normalmap)
        , bsdf_(new PerfectRefract(eta))
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Copy to be able to alter normal
        Primitive::Intersection isectlocal = isect;

        if (!normalmap_.empty())
        {
            MapNormal(normalmap_, isectlocal, dot(wi, isectlocal.n) < 0);
        }

        // TODO: add support for ray differentials
        float3 c = colormap_.empty() ? color_ : texturesys_.Sample(colormap_, isect.uv, float2(0,0));
        return c * bsdf_->Sample(isectlocal, sample, wi, wo, pdf);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Copy to be able to alter normal
        Primitive::Intersection isectlocal = isect;

        if (!normalmap_.empty())
        {
            MapNormal(normalmap_, isectlocal, dot(wi, isectlocal.n) < 0);
        }

        // TODO: add support for ray differentials
        float3 c = colormap_.empty() ? color_ : texturesys_.Sample(colormap_, isect.uv, float2(0,0));
        return c * bsdf_->Evaluate(isectlocal, wi, wo);
    }

    // Color
    float3 color_;
    // Diffuse map
    std::string colormap_;
    // Normal map
    std::string normalmap_;
    // BSDF
    std::unique_ptr<Bsdf> bsdf_;
};

#endif // MATTE_H