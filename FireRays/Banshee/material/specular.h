#ifndef SPECULAR_H
#define SPECULAR_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/perfect_refract.h"
#include "../bsdf/perfect_reflect.h"

///< Refract material for glass like objects
///<
class Specular : public Material
{
public:
    // If color map is specified it is used as a refraction color, otherwise specified color value is used
    Specular (TextureSystem const& texturesys, 
        float eta,
        float3 const& color, 
        std::string const& colormap = "",
        std::string const& normalmap = "")
        : Material(texturesys)
        , color_(color)
        , colormap_(colormap)
        , normalmap_(normalmap)
        , refractbsdf_(new PerfectRefract(eta))
        , reflectbsdf_(new PerfectReflect())
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Copy to be able to alter normal
        Primitive::Intersection isectlocal = isect;

        float ndotwi = dot(wi, isectlocal.n);

        if (!normalmap_.empty())
        {
            MapNormal(normalmap_, isectlocal, ndotwi < 0);
        }

        float etat = refractbsdf_->eta_;
        float etai = 1.f;


        if (ndotwi < 0)
        {
            float etat = 1.f;
            float etai = refractbsdf_->eta_;
            ndotwi = -ndotwi;
        }

        float sint = etai / etat * sqrtf(std::max(0.f, 1.f - ndotwi*ndotwi));
        float3 c = colormap_.empty() ? color_ : texturesys_.Sample(colormap_, isect.uv, float2(0,0));

        if (sint > 1.f)
        {
            // TIR
            return c*reflectbsdf_->Sample(isectlocal, sample, wi, wo, pdf);
        }
        else
        {
            float cost = sqrtf(std::max(0.f, 1.f - sint*sint));  
            float r = FresnelDielectricReflectivity(ndotwi, cost, etai, etat);

            float rnd = rand_float();

            if (rnd < r)
            {
                return c*reflectbsdf_->Sample(isectlocal, sample, wi, wo, pdf);
            }
            else
            {
                return c*refractbsdf_->Sample(isectlocal, sample, wi, wo, pdf);
            }
        }
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Copy to be able to alter normal
        Primitive::Intersection isectlocal = isect;

        float ndotwi = dot(wi, isectlocal.n);

        if (!normalmap_.empty())
        {
            MapNormal(normalmap_, isectlocal, ndotwi < 0);
        }

        float etat = refractbsdf_->eta_;
        float etai = 1.f;

        if (ndotwi < 0)
        {
            float etat = 1.f;
            float etai = refractbsdf_->eta_;
            ndotwi = -ndotwi;
        }

        float sint = etai / etat * sqrtf(std::max(0.f, 1.f - ndotwi*ndotwi));
        float3 c = colormap_.empty() ? color_ : texturesys_.Sample(colormap_, isect.uv, float2(0,0));

        if (sint > 1.f)
        {
            // TIR
            return c*reflectbsdf_->Evaluate(isectlocal, wi, wo);
        }
        else
        {
            float cost = sqrtf(std::max(0.f, 1.f - sint*sint));  
            float r = FresnelDielectricReflectivity(ndotwi, cost, etai, etat);

            float rnd = rand_float();

            if (rnd < r)
            {
                return c*reflectbsdf_->Evaluate(isectlocal, wi, wo);
            }
            else
            {
                return c*refractbsdf_->Evaluate(isectlocal, wi, wo);
            }
        }
    }

    // Color
    float3 color_;
    // Diffuse map
    std::string colormap_;
    // Normal map
    std::string normalmap_;

    // BSDF
    std::unique_ptr<PerfectReflect> reflectbsdf_;
    std::unique_ptr<PerfectRefract> refractbsdf_;
};

#endif // SPECULAR_H