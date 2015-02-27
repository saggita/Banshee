/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/
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
    OrenNayar(
              // Texture system
              TextureSystem const& texturesys,
              // Diffuse color
              float3 kd = float3(1.f, 1.f, 1.f),
              // Roughness value
              float  kr = 1.f,
              // Diffuse map
              std::string const& kdmap = "",
              // Roughness map
              std::string const& krmap = "",
              // Normal map
              std::string const& nmap = ""
              )
    : Bsdf(texturesys, REFLECTION | DIFFUSE)
    , kd_(kd)
    , kr_(kr)
    , kdmap_(kdmap)
    , krmap_(krmap)
    , nmap_(nmap)
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
        float3 n = dot(wi, isect.n) >= 0.f ? isectlocal.n : -isectlocal.n;
        
        // Map random sample to hemisphere getting cosine weigted distribution
        wo = map_to_hemisphere(n, sample, 1.f);
        
        // Normalization multiplier
        float invpi = 1.f / PI;
        
        // PDF proportional to cos
        pdf = dot(n, wo) * invpi;
        
        // Evaluate
        return Evaluate(isect, wi, wo);
    }
    
    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Return 0 if wo and wi are on different sides
        float sameside = dot(wi, isect.n) * dot(wo, isect.n) ;
        if (sameside < 0.f)
            return float3(0, 0, 0);
        
        // Backup for normal mapping
        Primitive::Intersection isectlocal = isect;
        
        // Alter normal if needed
        // TODO: fix tangents as well
        MAP_NORMAL(nmap_, isectlocal);
        
        float3 n = isectlocal.n;
        float3 s = isectlocal.dpdu;
        float3 t = isectlocal.dpdv;
        
        // Revert normal based on ORIGINAL normal, not mapped one
        if (dot(wi, isect.n) < 0.f)
        {
            n = -n;
            s = -s;
            t = -t;
        }
        
        // Get roughness value
        float kr = GET_VALUE(kr_, krmap_, isectlocal.uv).x;
        float invpi = 1.f / PI;
        float r2 = kr*kr;
        
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
        
        float3 kd = GET_VALUE(kd_, kdmap_, isect.uv);
        
        return kd * float3(invpi, invpi, invpi) * (a + b * maxcos * sin_alpha * tan_beta);
    }
    
    // Return pdf for wo to be sampled for wi
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // If wi and wo are on the same side of the surface
        float sameside = dot(wi, isect.n) * dot(wo, isect.n);
        
        if (sameside > 0.f)
        {
            float3 n = dot(wi, isect.n) >= 0.f ? isect.n : -isect.n;
            
            float invpi = 1.f / PI;
            
            return dot(n, wo) * invpi;
        }
        else
        {
            return 0.f;
        }
    }

private:
    // Diffuse color
    float3 kd_;
    // Roughness value
    float kr_;
    // Diffuse texture
    std::string kdmap_;
    // Roughness map
    std::string krmap_;
    // Normal texture
    std::string nmap_;
};


#endif // ORENNAYAR_H