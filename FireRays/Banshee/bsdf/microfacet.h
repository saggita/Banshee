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
#ifndef MICROFACET_H
#define MICROFACET_H

#include "../math/mathutils.h"

#include "bsdf.h"
#include "fresnel.h"
#include "microfacet_distribution.h"

///< Torrance-Sparrow microfacet model. A physically based specular BRDF is based on microfacet theory,
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
               // Texture system
               TextureSystem const& texturesys,
               // Refractive index
               float eta = 1.f,
               // Reflection color
               float3 ks = float3(0.7f, 0.7f, 0.7f),
               // Reflection map
               std::string const& ksmap = "",
               // Normal map
               std::string const& nmap = "",
               // Fresnel component
               Fresnel* fresnel = nullptr,
               // Microfacet distribution
               MicrofacetDistribution* md = nullptr
               )
    : Bsdf(texturesys, REFLECTION | GLOSSY)
    , eta_(eta)
    , ks_(ks)
    , ksmap_(ksmap)
    , nmap_(nmap)
    , fresnel_(fresnel ? fresnel : new FresnelDielectric())
    , md_(md ? md : new BlinnDistribution(10.f))
    {
    }
    
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(ShapeBundle::Hit& hit, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        ShapeBundle::Hit hitlocal = hit;
        
        // Revert normal if needed
        if (dot(hit.n, wi) < 0.f)
        {
            hitlocal.n = -hitlocal.n;
            hitlocal.dpdu = -hitlocal.dpdu;
            hitlocal.dpdv = -hitlocal.dpdv;
        }
        
        // Sample distribution
        md_->Sample(hitlocal, sample, wi, wo, pdf);
        
        // Evaluate
        return Evaluate(hit, wi, wo);
    }
    
    // Evaluate combined BSDF value
    float3 Evaluate(ShapeBundle::Hit& hit, float3 const& wi, float3 const& wo) const
    {
        // Return 0 if wo and wi are on different sides
        float sameside = dot(wi, hit.n) * dot(wo, hit.n);
        
        if (sameside < 0.f)
            return float3(0, 0, 0);
        
        ShapeBundle::Hit hitlocal = hit;
        
        // Revert normal if needed
        float3 n = dot(hit.n, wi) < 0.f ? -hitlocal.n : hitlocal.n;
        
        // Incident and reflected zenith angles
        float cos_theta_o = dot(n, wo);
        float cos_theta_i = dot(n, wi);
        
        if (cos_theta_i == 0.f || cos_theta_o == 0.f)
            return float3(0.f, 0.f, 0.f);
        
        // Calc halfway vector
        float3 wh = normalize(wi + wo);
        
        // Calc Fresnel for wh faced microfacets
        float fresnel = fresnel_->Evaluate(1.f, eta_, dot(wi, wh));
        
        float3 ks = GET_VALUE(ks_, ksmap_, hit.uv);
        
        // F(wi,wo) = D(wh)*Fresnel(wh, n)*G(wi, wo, n)/(4 * cos_theta_i * cos_theta_o)
        return ks * (md_->D(wh, n) * md_->G(wi, wo, wh, n) * fresnel / (4.f * cos_theta_i * cos_theta_o));
    }
    
    // Return pdf for wo to be sampled for wi
    float GetPdf(ShapeBundle::Hit& hit, float3 const& wi, float3 const& wo) const
    {
        // Return 0 if wo and wi are on different sides
        float sameside = dot(wi, hit.n) * dot(wo, hit.n);
        if (sameside < 0.f)
            return 0.f;
        
        ShapeBundle::Hit hitlocal = hit;
        
        if (dot(hitlocal.n, wi) < 0)
        {
            hitlocal.n = -hitlocal.n;
            hitlocal.dpdu = -hitlocal.dpdu;
            hitlocal.dpdv = -hitlocal.dpdv;
        }
        
        return md_->GetPdf(hitlocal, wi, wo);
    }
    
private:
    
    // Reflection color
    float3 ks_;
    // IOR
    float eta_;
    // Diffuse texture
    std::string ksmap_;
    // Normal texture
    std::string nmap_;
    // Fresnel component
    std::unique_ptr<Fresnel> fresnel_;
    // Microfacet distribution
    std::unique_ptr<MicrofacetDistribution> md_;
};





#endif // MICROFACET_H
