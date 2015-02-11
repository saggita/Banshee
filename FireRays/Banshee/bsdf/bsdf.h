#ifndef BSDF_H
#define BSDF_H

#include "../primitive/primitive.h"
#include "../texture/texturesystem.h"

#include <string>

// The macro uses texture to get value if available, otherwise sets to constant value
#define GET_VALUE(val,tex,uv) ((tex).empty()) ? (val):texturesys_.Sample((tex), (uv), float2(0,0))
#define MAP_NORMAL(tex, isect) if(!(tex).empty())MapNormal((tex),(isect))

///< Bsdf is an abstraction for all BSDFs in the system
///<
class Bsdf
{
public:
    enum
    {
        REFLECTION = (1 << 0),
        TRANSMISSION = (1 << 1),
        DIFFUSE = (1 << 2),
        SPECULAR = (1 << 3),
        GLOSSY = (1 << 4),
        ALL_REFLECTION = REFLECTION | DIFFUSE | SPECULAR | GLOSSY,
        ALL_TRANSMISSION =  TRANSMISSION | DIFFUSE | SPECULAR | GLOSSY,
        ALL = REFLECTION | TRANSMISSION | DIFFUSE | SPECULAR | GLOSSY
    };
    
    // Constructor
    Bsdf(TextureSystem const& texturesys, int type)
    : texturesys_(texturesys)
    , type_(type)
    {
    }
    
    // Destructor
    virtual ~Bsdf() {}

    // Sample material and return outgoing ray direction along with combined BSDF value
    virtual float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const = 0;

    // Evaluate combined BSDF value
    virtual float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;
    
    // PDF of a given direction sampled from isect.p
    virtual float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;
    
    // Get BSDF type
    int GetType() const { return type_; }
    
protected:
    // Apply normal mapping
    void MapNormal(std::string const& nmap, Primitive::Intersection& isect) const;
    
    // Texture system interface
    TextureSystem const& texturesys_;
    
    // BSDF type
    int type_;
};

inline void Bsdf::MapNormal(std::string const& nmap, Primitive::Intersection& isect) const
{
    // We dont need bilinear interpolation while fetching normals
    // Use point instead
    TextureSystem::Options opts(TextureSystem::Options::kPoint);
    float3 normal = 2.f * texturesys_.Sample(nmap, isect.uv, float2(0,0), opts) - float3(1.f, 1.f, 1.f);
    isect.n = normalize(isect.n * normal.z + isect.dpdu * normal.x + isect.dpdv * normal.y);
}



#endif // BSDF_H