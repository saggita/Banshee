#ifndef PERFECT_REFRACT_H
#define PERFECT_REFRACT_H

#include "../math/mathutils.h"

#include "bsdf.h"
///< In optics, refraction is a phenomenon that often occurs when waves travel 
///< from a medium with a given refractive index to a medium with another at an oblique angle.
///< At the boundary between the media, the wave's phase velocity is altered, usually causing a change in direction. 
///< (c) Wikipedia
///<
class PerfectRefract : public Bsdf
{
public:

    // Constructor
    PerfectRefract(
                   // Texture system
                   TextureSystem const& texturesys,
                   // Refractive index
                   float eta,
                   // Specular refract color
                   float3 ks = float3(1.f, 1.f, 1.f),
                   // Specular refract map
                   std::string const& ksmap = "",
                   // Normal map
                   std::string const& nmap = "",
                   // Indicates whether Fresnel should be used and which one
                   // Leave nullptr to disable Fresnel effect
                   Fresnel* fresnel = nullptr
                   )
    : Bsdf(texturesys, TRANSMISSION | SPECULAR)
    , eta_(eta)
    , ks_(ks)
    , ksmap_(ksmap)
    , nmap_(nmap)
    , fresnel_(fresnel)
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {

        // Backup for normal mapping
        Primitive::Intersection isectlocal = isect;
        
        // Alter normal if needed
        // TODO: fix tangents as well
        MAP_NORMAL(nmap_, isectlocal);
        
        // Revert normal based on ORIGINAL normal, not mapped one
        float3 n;
        float eta;
        float ndotwi = dot(wi, isectlocal.n);
        
        // Revert normal and eta if needed
        if (ndotwi >= 0.f)
        {
            n = isect.n;
            eta = eta_;
        }
        else
        {
            n = -isect.n;
            eta = 1 / eta_;
            ndotwi = -ndotwi;
        }

        // Use original isect.n here to make sure IOR ordering is correct
        // as we could have reverted normal and eta
        float reflectance = fresnel_ ? fresnel_->Evaluate(1.f, eta_, dot(wi, isect.n)) : 0.f;

        // If not TIR return transmitance BSDF
        if (reflectance < 1.f)
        {
            // This is > 0 as reflectance  < 1.f
            float q = std::max(0.f, 1.f - (1 - ndotwi * ndotwi) / (eta*eta));
            
            // Transmitted ray
            wo = normalize(- (1.f / eta) * wi - (sqrtf(q) - ndotwi / eta)*n);
            
            // TODO: fix this
            assert(!has_nans(wo));
            
            // PDF is infinite at that point, but deltas are going to cancel out while evaluating
            // so set it to 1.f
            pdf = 1.f;
            
            // Get refract color value
            float3 ks = GET_VALUE(ks_, ksmap_, isect.uv);
            
            // Account for reflectance
            return ndotwi > 0.f ? eta*eta*(1.f - reflectance)*ks*(1.f / ndotwi) : float3(0.f, 0.f, 0.f);
        }
        else
        {
            // Total internal reflection, so return 0.f
            pdf = 0.f;
            
            return float3(0.f, 0.f, 0.f);
        }
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Delta function, return 0
        return float3(0.f, 0.f, 0.f);
    }
    
    // Return pdf for wo to be sampled for wi
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Delta function, return 0
        return 0.f;
    }

    // Specular refract color
    float3 ks_;
    // Specular refract texture
    std::string ksmap_;
    // Normal texture
    std::string nmap_;
    // Fresnel component
    std::unique_ptr<Fresnel> fresnel_;
    // Refractive index
    float eta_;
};


#endif // PERFECT_REFRACT_H