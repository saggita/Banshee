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
                   float3 ks = float3(0.7f, 0.7f, 0.7f),
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
    float3 Sample(ShapeBundle::Hit& hit, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Revert normal based on ORIGINAL normal, not mapped one
        float3 n;
        float eta;
        float ndotwi = dot(wi, hit.n);
        
        // Revert normal and eta if needed
        if (ndotwi >= 0.f)
        {
            n = hit.n;
            eta = eta_;
        }
        else
        {
            n = -hit.n;
            eta = 1 / eta_;
            ndotwi = -ndotwi;
        }

        // Use original hit.n here to make sure IOR ordering is correct
        // as we could have reverted normal and eta
        float reflectance = fresnel_ ? fresnel_->Evaluate(1.f, eta_, dot(wi, hit.n)) : 0.f;

        // If not TIR return transmitance BSDF
        if (reflectance < 1.f)
        {
            // This is > 0 as reflectance  < 1.f
            float q = std::max(0.f, 1.f - (1 - ndotwi * ndotwi) / (eta*eta));
            
            // Transmitted ray
            wo = normalize(- (1.f / eta) * wi - (sqrtf(q) - ndotwi / eta)*n);
            
            // PDF is infinite at that point, but deltas are going to cancel out while evaluating
            // so set it to 1.f
            pdf = 1.f;
            
            // Get refract color value
            float3 ks = GET_VALUE(ks_, ksmap_, hit.uv);
            
            // Account for reflectance
            return ndotwi > FLT_EPSILON ? (eta*eta*(1.f - reflectance)*ks*(1.f / ndotwi)) : float3(0.f, 0.f, 0.f);
        }
        else
        {
            // Total internal reflection, so return 0.f
            pdf = 0.f;
            
            return float3(0.f, 0.f, 0.f);
        }
    }

    // Evaluate combined BSDF value
    float3 Evaluate(ShapeBundle::Hit& hit, float3 const& wi, float3 const& wo) const
    {
        // Delta function, return 0
        return float3(0.f, 0.f, 0.f);
    }
    
    // Return pdf for wo to be sampled for wi
    float GetPdf(ShapeBundle::Hit& hit, float3 const& wi, float3 const& wo) const
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