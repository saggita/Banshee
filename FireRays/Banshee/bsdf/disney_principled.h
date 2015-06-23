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
#ifndef DISNEY_PRINCIPLED_H
#define DISNEY_PRINCIPLED_H

#include "../math/mathutils.h"

#include "bsdf.h"

///< Helper functions
///< Shlick Fresnel term approx for Disney BRDF
float SchlickFresnel(float u)
{
    float m = clamp(1.f - u, 0.f, 1.f);
    float m2 = m * m;
    return m2 * m2 * m;
}

///<
float SchlickFresnelEta(float eta, float cosi)
{
    float f = ((1.f - eta) / (1.f + eta)) * ((1.f - eta) / (1.f + eta));
    float m = 1.f - fabs(cosi);
    float m2 = m*m;
    return f + (1.f - f) * m2 * m2 * m;
}

///< GTR1 mf distribution
float Gtr1(float ndoth, float a)
{
    if (a >= 1.f)
        return 1.f / PI;
    
    float a2 = a * a;
    float t = 1.f + (a2 - 1.f) * ndoth * ndoth;
    
    return (a2 - 1.f) / (M_PI * log(a2) * t);
}

///< Anisotropic mf distribution
float Gtr2Aniso(
                float ndoth,
                float hdotx,
                float hdoty,
                float ax,
                float ay
                )
{
    return clamp(
                 1.f / (M_PI * ax * ay *
                        ((hdotx / ax)*(hdotx / ax) + (hdoty / ay)*(hdoty / ay) + ndoth * ndoth) *
                        ((hdotx / ax)*(hdotx / ax) + (hdoty / ay)*(hdoty / ay) + ndoth * ndoth)),
                 0.f, 10.f); // A trick to avoid fireflies
}

float SmithGggx(float ndotv, float alphag)
{
    float a = alphag * alphag;
    float b = ndotv * ndotv;
    return 1.f / (ndotv + std::sqrt(a + b - a * b));
}

///< The empirical model developed at Disney and described here:
///< https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
///<
class DisneyPrincipled : public Bsdf
{
public:
    ///< This BRDF has tons of parameters, better move them out
    struct Params
    {
        float metallic_;
        float subsurface_;
        float specular_;
        float speculartint_;
        float sheen_;
        float sheentint_;
        float clearcoat_;
        float clearcoatgloss_;
        float aniso_;
        
        Params()
        : metallic_ (0.f)
        , subsurface_(0.f)
        , specular_(0.5f)
        , speculartint_(0.5f)
        , sheen_(0.f)
        , sheentint_(0.f)
        , clearcoat_(0.5f)
        , clearcoatgloss_(0.f)
        , aniso_(0.f)
        {
        }
    };
    
    DisneyPrincipled(
              // Texture system
              TextureSystem const& texturesys,
              // Refractive index
              float eta,
              // Diffuse color
              float3 kd = float3(0.7f, 0.7f, 0.7f),
              // BRDF parameters
              Params params = Params(),
              // Diffuse map
              std::string const& kdmap = "",
              // Roughness texture
              std::string const& krmap = "",
              // Normal map
              std::string const& nmap = ""
              )
    : Bsdf(texturesys, REFLECTION | DIFFUSE)
    , kd_(kd)
    , kr_(kr)
    , params_(params)
    , kdmap_(kdmap)
    , nmap_(nmap)
    {
    }
    
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
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
    float3 Evaluate(Primitive::Intersection& isect, float3 const& wi, float3 const& wo) const
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
        float3 kd = GET_VALUE(kd_, kdmap_, isect.uv);
        
        
    }
    
    // Return pdf for wo to be sampled for wi
    float Pdf(Primitive::Intersection& isect, float3 const& wi, float3 const& wo) const
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
    // Refractive index
    float eta_;
    // Roughness
    float kr_;
    // Diffuse texture
    std::string kdmap_;
    // Roughness map
    std::string krmap_;
    // Normal texture
    std::string nmap_;
    // BRDF parameters
    Params params_;
};


#endif // DISNEY_PRINCIPLED_H