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
#ifndef PERFECT_SPECULAR_H
#define PERFECT_SPECULAR_H

#include "../math/mathutils.h"

#include "bsdf.h"
#include "fresnel.h"

///< Specular reflection is the mirror-like reflection of light (or of other kinds of wave) from a surface, 
///< in which light from a single incoming direction (a ray) is reflected into a single outgoing direction. 
///< Such behavior is described by the law of reflection, which states that the direction of incoming light (the incident ray), 
///< and the direction of outgoing light reflected (the reflected ray) make the same angle with respect to the surface normal, 
///< thus the angle of incidence equals the angle of reflection (\theta _i = \theta _r in the figure), 
///< and that the incident, normal, and reflected directions are coplanar.
///< (c) Wikipedia
///<
class PerfectReflect : public Bsdf
{
public:
    PerfectReflect(
                   // Texture system
                   TextureSystem const& texturesys,
                   // Refractive index
                   float eta,
                   // Specular reflect color
                   float3 ks = float3(1.f, 1.f, 1.f),
                   // Specular reflect map
                   std::string const& ksmap = "",
                   // Normal map
                   std::string const& nmap = "",
                   // Indicates whether Fresnel should be used and which one
                   // Leave nullptr to disable Fresnel effect
                   Fresnel* fresnel = nullptr
    )
    : Bsdf(texturesys, REFLECTION | SPECULAR)
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
        float3 n = dot(wi, isect.n) >= 0.f ? isectlocal.n : -isectlocal.n;
        
        // Mirror reflect wi
        wo = normalize(2.f * dot(n, wi) * n - wi);
        
        // PDF is infinite at that point, but deltas are going to cancel out while evaluating
        // so set it to 1.f
        pdf = 1.f;
        
        // Get reflect color value
        float3 ks = GET_VALUE(ks_, ksmap_, isect.uv);

        // If Fresnel is used calculate Fresnel reflectance using ORIGINAL normal to
        // correctly determine reflected and transmitted parts
        float reflectance = fresnel_ ? fresnel_->Evaluate(1.f, eta_, dot(wi, isect.n)) : 1.f;
        
        float ndotwi = dot(n, wi);
        
        // Return reflectance value
        return ndotwi > 0.01f ? reflectance * ks * (1.f / ndotwi) : float3(0.f, 0.f, 0.f);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Delta function, so 0.f
        return float3(0.f, 0.f, 0.f);
    }
    
    // Return pdf for wo to be sampled for wi
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Delta function, so 0.f
        return 0.f;
    }
    
    // Specular reflect color
    float3 ks_;
    // Specular reflect texture
    std::string ksmap_;
    // Normal texture
    std::string nmap_;
    // Fresnel component
    std::unique_ptr<Fresnel> fresnel_;
    // Refractive index
    float eta_;
};


#endif // PERFECT_SPECULAR_H