#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>

#include "../texture/texturesystem.h"
#include "../primitive/primitive.h"


///< Material is an interface for the renderer to call
///< when handling intersections. It encapsulates all
///< the surface appearance evaluations.
///<
class Material
{
public:
    Material ()
    {}

    // Destructor
    virtual ~Material() {}

    // Sample material and return outgoing ray direction along with combined BSDF value and sampled BSDF type
    virtual float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const = 0;

    // Evaluate combined BSDF value
    virtual float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;
    
    // PDF of a given direction sampled from isect.p
    virtual float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;

    // Indicate whether the materials has emission component and will be used for direct light evaluation
    virtual bool emissive() const { return false; }

    // Emission component of the material
    virtual float3 Le(Primitive::SampleData const& sampledata, float3 const& wo) const { return float3(0,0,0); }
};
#endif // MATERIAL_H