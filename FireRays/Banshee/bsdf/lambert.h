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
#ifndef LAMBERT_H
#define LAMBERT_H

#include "../math/mathutils.h"

#include "bsdf.h"
///< Lambertian reflectance is the property that defines an ideal "matte" or diffusely reflecting surface. 
///< The apparent brightness of such a surface to an observer is the same regardless of the observer's angle of view. 
///< More technically, the surface's luminance is isotropic, and the luminous intensity obeys Lambert's cosine law. 
///< Lambertian reflectance is named after Johann Heinrich Lambert, who introduced the concept of perfect diffusion in his 1760 book Photometria.
///< (c) Wikipedia
///<
class Lambert : public Bsdf
{
public:
    Lambert(
            // Texture system
            TextureSystem const& texturesys,
            // Diffuse color
            float3 kd = float3(1.f, 1.f, 1.f),
            // Diffuse map
            std::string const& kdmap = "",
            // Normal map
            std::string const& nmap = ""
            )
    : Bsdf(texturesys, REFLECTION | DIFFUSE)
    , kd_(kd)
    , kdmap_(kdmap)
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
        
        // Diffuse albedo
        float3 kd = GET_VALUE(kd_, kdmap_, isect.uv);
       
        // Return constant diffuse albedo
        return invpi * kd;
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // If wi and wo are on the same side of the surface return 1 / PI, otherwise 0.f
        float sameside = dot(wi, isect.n) * dot(wo, isect.n) ;
        
        if (sameside > 0.f)
        {
            float invpi = 1.f / PI;
            
            return invpi * kd_;
        }
        else
        {
            return float3(0.f, 0.f, 0.f);
        }
    }
    
    // Return pdf for wo to be sampled for wi
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // If wi and wo are on the same side of the surface return dot(n, wo) / PI, otherwise 0.f
        float sameside = dot(wi, isect.n) * dot(wo, isect.n) ;
        
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
    
    // Diffuse color
    float3 kd_;
    // Diffuse texture
    std::string kdmap_;
    // Normal texture
    std::string nmap_;
};


#endif // LAMBERT_H