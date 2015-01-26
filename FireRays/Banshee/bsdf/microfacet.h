#ifndef MICROFACET_H
#define MICROFACET_H

#include "../math/mathutils.h"


#include "bsdf.h"
#include "fresnel.h"


///< Interface for microfacet distribution for Microfacet BRDF
///< Returns the probability that microfacet has orientation w.
///<
class MicrofacetDistribution
{
public:
    // Destructor
    virtual ~MicrofacetDistribution(){}
    // w - microfacet orientation (normal), n - surface normal
    virtual float D(float3 const& w, float3 const& n) = 0;
};


///< Torrance-Sparrow microfacet model. A physically based specular BRDF is based on micro-facet theory,
///< which describe a surface is composed of many micro-facets and each micro-facet will only reflect light
///< in a single direction according to their normal(m):
///< F(wi,wo) = D(wh)*Fresnel(wh, n)*G(wi, wo, n)/(4 * cos_theta_i * cos_theta_o)
///<
class Microfacet : public Bsdf
{
public:
    // eta - refractive index of the object
    // fresnel - Fresnel component
    // md - microfacet distribution
    Microfacet(
               float eta,
               Fresnel* fresnel,
               MicrofacetDistribution* md
               )
    : fresnel_(fresnel)
    , eta_(eta)
    , md_(md)
    {
    }
    
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // will need to account for samling strategy later and provide a sampler
        float invpi = 1.f / PI;
        float3 n = dot(wi, isect.n) >= 0.f ? isect.n : -isect.n;
        wo = map_to_hemisphere(n, sample, 1.f);
        pdf = dot(n, wo) * invpi;
        return Evaluate(isect, wi, wo);
    }
    
    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        float3 n = isect.n;
        float3 s = isect.dpdu;
        float3 t = isect.dpdv;
        
        // Account for backfacing normal
        if (dot(wi, n) < 0.f)
        {
            n = -n;
            s = -s;
            t = -t;
        }
        
        // Incident and reflected zenith angles
        float cos_theta_o = dot(n, wo);
        float cos_theta_i = dot(n, wi);
        
        if (cos_theta_i == 0.f || cos_theta_o == 0.f)
            return float3(0.f, 0.f, 0.f);
        
        // Calc halfway vector
        float3 wh = normalize(wi + wo);
        
        // Calc Fresnel for wh faced microfacets
        float fresnel = fresnel_->Evaluate(1.f, eta_, dot(wi, wh));
        
        // F(wi,wo) = D(wh)*Fresnel(wh, n)*G(wi, wo, n)/(4 * cos_theta_i * cos_theta_o)
        return float3(1.f, 1.f, 1.f) * (md_->D(wh, n) * G(wi, wo, wh, n) * fresnel / (4.f * cos_theta_i * cos_theta_o));
    }
    
    // Geometric factor accounting for microfacet shadowing, masking and interreflections
    float G(float3 const& wi, float3 const& wo, float3 const& wh, float3 const& n ) const
    {
        float ndotwh = fabs(dot(n, wh));
        float ndotwo = fabs(dot(n, wo));
        float ndotwi = fabs(dot(n, wi));
        float wodotwh = fabs(dot(wo, wh));
        
        return std::min(1.f, std::min(2.f * ndotwh * ndotwo / wodotwh, 2.f * ndotwh * ndotwi / wodotwh));
    }
    
    // Roughness
    float roughness_;
    // IOR
    float eta_;
    // Fresnel component
    std::unique_ptr<Fresnel> fresnel_;
    // Microfacet distribution
    std::unique_ptr<MicrofacetDistribution> md_;
};


///< Blinn distribution of microfacets based on Gaussian distribution : D(wh) ~ (dot(wh, n)^e)
///< D(wh) = (e + 2) / (2*PI) * dot(wh,n)^e
///<
class BlinnDistribution: public MicrofacetDistribution
{
public:
    // Constructor
    BlinnDistribution(float e)
    : e_(e)
    {
    }
    
    // Distribution fucntiom
    float D(float3 const& w, float3 const& n)
    {
        float ndotw = fabs(dot(n, w));
        return (1.f / (2*PI)) * (e_ + 2) * powf(ndotw, e_);
    }
    
    // Exponent
    float e_;
};


#endif // MICROFACET_H