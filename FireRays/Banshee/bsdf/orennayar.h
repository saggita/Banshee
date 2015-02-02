#ifndef ORENNAYAR_H
#define ORENNAYAR_H

#include "../math/mathutils.h"

#include "bsdf.h"
///< The Oren–Nayar reflectance model,[1] developed by Michael Oren and Shree K. Nayar,
///< is a reflectivity model for diffuse reflection from rough surfaces.
///< It has been shown to accurately predict the appearance of a wide range of  natural surfaces, such as concrete, plaster, sand, etc.
///< Fr(wi, wo) = (rho / PI) * (A + B * max(0, cos(phi_i - phi_o))*sin(alpha)*tan(beta);
///< alpha = max(theta_i, theta_o)
///< beta = min(theta_i, theta_o)
///< (c) Wikipedia
///<
class OrenNayar : public Bsdf
{
public:
    OrenNayar(float roughness)
    : roughness_(roughness)
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
        
        //
        if (dot(wi, n) < 0.f)
        {
            n = -n;
            s = -s;
            t = -t;
        }
        
        float invpi = 1.f / PI;
        float r2 = roughness_*roughness_;
        
        // A
        float a = 1.f - 0.5f * r2 / (r2 + 0.33f);
        // B
        float b = 0.45f * r2 / (r2 + 0.09f);
        
        float cos_theta_i = dot(n, wi);
        float cos_theta_o = dot(n, wo);
        float sin_theta_i = sqrtf(1.f - cos_theta_i*cos_theta_i);
        float sin_theta_o = sqrtf(1.f - cos_theta_o*cos_theta_o);
        
        float cos_phi_i = dot(wi, s);
        float cos_phi_o = dot(wo, s);
        float sin_phi_i = sqrtf(1.f - cos_phi_i*cos_phi_i);
        float sin_phi_o = sqrtf(1.f - cos_phi_o*cos_phi_o);

        // max(0, cos(phi_i - phi_o))
        float maxcos = 0;
        
        if (sin_theta_i > 0 && sin_theta_o > 0)
        {
            maxcos = std::max(0.f, cos_phi_i*cos_phi_o + sin_phi_i*sin_phi_o);
        }
        
        float sin_alpha = 0;
        float tan_beta = 0;
        
        if (cos_theta_i > cos_theta_o)
        {
            sin_alpha = sin_theta_o;
            tan_beta = sin_theta_i / cos_theta_i;
        }
        else
        {
            sin_alpha = sin_theta_i;
            tan_beta = sin_theta_o / cos_theta_o;
        }
        
        return float3(invpi, invpi, invpi) * (a + b * maxcos * sin_alpha * tan_beta);
    }
    
    // Return pdf for wo to be sampled for wi
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        float invpi = 1.f / PI;
        float3 n = dot(wi, isect.n) >= 0.f ? isect.n : -isect.n;
        return dot(n, wo) * invpi;
    }

private:
    // Roughness
    float roughness_;
};


#endif // ORENNAYAR_H