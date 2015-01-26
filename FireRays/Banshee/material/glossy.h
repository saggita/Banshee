#ifndef GLOSSY_H
#define GLOSSY_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/microfacet.h"
#include "../bsdf/fresnel.h"

///< Matte material is providing the look of a diffuse surfaces
///<
class Glossy : public Material
{
public:
    // If diffuse map is specified it is used as a diffuse color, otherwise diffuse color is used
    Glossy (TextureSystem const& texturesys,
           float3 const& specular,
           float eta,
           float roughness,
           std::string const& specularmap = "",
           std::string const& normalmap = "")
    : Material(texturesys)
    , specular_(specular)
    , specularmap_(specularmap)
    , normalmap_(normalmap)
    , bsdf_(new Microfacet(eta, new FresnelDielectric(), new BlinnDistribution(roughness)))
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
        float  ndotwi = fabs(dot(isectlocal.n, wi));
        float3 ks = specularmap_.empty() ? specular_ : texturesys_.Sample(specularmap_, isect.uv, float2(0,0));
        return ks * bsdf_->Sample(isectlocal, sample, wi, wo, pdf) * ndotwi;
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
        float  ndotwi = fabs(dot(isectlocal.n, wi));
        float3 ks = specularmap_.empty() ? specular_ : texturesys_.Sample(specularmap_, isect.uv, float2(0,0));
        return ks * bsdf_->Evaluate(isectlocal, wi, wo) * ndotwi;
    }
    
    // Specular color
    float3 specular_;
    // Specular color map
    std::string specularmap_;
    // Normal map
    std::string normalmap_;
    // BSDF
    std::unique_ptr<Bsdf> bsdf_;
};

#endif // GLOSSY_H