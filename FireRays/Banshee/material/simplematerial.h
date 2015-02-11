#ifndef SIMPLEMATERIAL_H
#define SIMPLEMATERIAL_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/bsdf.h"

///< SimpleMaterial consists of a single BSDF
///<
class SimpleMaterial : public Material
{
public:
    // If diffuse map is specified it is used as a diffuse color, otherwise diffuse color is used
    SimpleMaterial(Bsdf* bsdf)
    : bsdf_(bsdf)
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const
    {
        type = bsdf_->GetType();
        return bsdf_->Sample(isect, sample, wi, wo, pdf);
    }
    
    // PDF of a given direction sampled from isect.p
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        return bsdf_->Pdf(isect, wi, wo);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        return bsdf_->Evaluate(isect, wi, wo);
    }
    
private:
    // BSDF
    std::unique_ptr<Bsdf> bsdf_;
};

#endif // SIMPLEMATERIAL_H