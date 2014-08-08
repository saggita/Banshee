#ifndef MATERIAL_H
#define MATERIAL_H

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
    virtual float3 Sample(Primitive::Intersection const& isect, float3 const& wi, float3& wo, float& pdf) const = 0;

    // Evaluate combined BSDF value
    virtual float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;

protected:
    TextureSystem const& texturesys_;
};

#endif // MATERIAL_H