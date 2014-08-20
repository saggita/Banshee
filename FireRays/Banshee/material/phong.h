#ifndef PHONG_H
#define PHONG_H

#include "material.h"
#include "../bsdf/lambert.h"
#include "../bsdf/perfect_specular.h"

///< Phong material provides the combination of Lambert + specular BRDFs
///< and is supposed to be used in simple renders and imported material translations
///<
class Phong : public Material
{
public:
    // If diffuse map is specified it is used as a diffuse color, otherwise diffuse color is used
    Phong (TextureSystem const& texturesys, 
        float3 const& diffuse, 
        float3 const& specular, 
        std::string const& diffusemap = "",
        std::string const& normalmap = "")
        : Material(texturesys)
        , diffuse_(diffuse)
        , specular_(specular)
        , diffusemap_(diffusemap)
        , normalmap_(normalmap)
        , diffusebsdf_(new Lambert())
        , specularbsdf_(new PerfectSpecular())
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float3 const& wi, float3& wo, float& pdf) const
    {
        // Copy to be able to alter normal
        Primitive::Intersection isectlocal = isect;

        if (!normalmap_.empty())
        {
            MapNormal(normalmap_, isectlocal);
        }

        // TODO: add support for ray differentials
        float  ndotwi = dot(isectlocal.n, wi);

        if (ndotwi > 0.f)
        {
            float3 kd = diffusemap_.empty() ? diffuse_ : texturesys_.Sample(diffusemap_, isect.uv, float2(0,0));
            float3 ks = specular_;
            float3 dwo, swo;
            float3 f = kd * diffusebsdf_->Sample(isectlocal, wi, dwo, pdf) * ndotwi + ks * specularbsdf_->Sample(isectlocal, wi, swo, pdf);

            // Fake for now
            float kkt = ks.sqnorm() + kd.sqnorm();
            float kkd = kd.sqnorm();
            float prob = kkd / kkt;
            wo = rand_float() < prob ? dwo : swo;

            return f;
        }

        return float3(0,0,0);
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
        float  ndotwi = dot(isectlocal.n, wi);

        if (ndotwi > 0.f)
        {
            float3 kd = diffusemap_.empty() ? diffuse_ : texturesys_.Sample(diffusemap_, isect.uv, float2(0,0));
            float3 ks = specular_;
            float3 f = kd * diffusebsdf_->Evaluate(isectlocal, wi, wo) * ndotwi + ks * specularbsdf_->Evaluate(isectlocal, wi, wo);

            return f;
        }

        return float3(0,0,0);
    }

    // Diffuse color
    float3 diffuse_;
    // Specular color
    float3 specular_;
    // Diffuse map
    std::string diffusemap_;
    // Normal map
    std::string normalmap_;
    // BSDFs
    std::unique_ptr<Bsdf> diffusebsdf_;
    std::unique_ptr<Bsdf> specularbsdf_;
};

#endif // PHONG_H