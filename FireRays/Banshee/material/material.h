#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>

#include "../texture/texturesystem.h"
#include "../primitive/primitive.h"
#include "../bsdf/bsdf.h"


///< Material is an interface for the renderer to call
///< when handling intersections. It encapsulates all
///< the surface appearance evaluations.
///<
class Material : public Bsdf
{
public:
    Material (TextureSystem const& texturesys)
        : texturesys_(texturesys)
    {
    }

    // Destructor
    virtual ~Material() {}

    // Sample material and return outgoing ray direction along with combined BSDF value
    virtual float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const = 0;

    // Evaluate combined BSDF value
    virtual float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;

    // Indicate whether the materials has emission component and will be used for direct light evaluation
    virtual bool emissive() const { return false; }

    // Emission component of the material
    virtual float3 Le(Primitive::SampleData const& sampledata, float3 const& wo) const { return float3(0,0,0); }

protected:
    // Function to support normal mapping
    virtual void MapNormal(std::string const& nmap, Primitive::Intersection& isect, bool flipnormal) const;

    TextureSystem const& texturesys_;
};

inline void Material::MapNormal(std::string const& nmap, Primitive::Intersection& isect, bool flipnormal) const
{
    // We dont need bilinear interpolation while fetching normals
    // Use point instead
    TextureSystem::Options opts(TextureSystem::Options::kPoint);
    float3 normal = 2.f * texturesys_.Sample(nmap, isect.uv, float2(0,0), opts) - float3(1.f, 1.f, 1.f);
    isect.n = flipnormal ? normalize(isect.n * normal.z - isect.dpdu * normal.x - isect.dpdv * normal.y) : normalize(isect.n * normal.z + isect.dpdu * normal.x + isect.dpdv * normal.y);
}

#endif // MATERIAL_H