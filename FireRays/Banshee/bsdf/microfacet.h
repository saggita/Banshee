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


///< Interface for microfacet distribution for Microfacet BRDF
///< Returns the probability that microfacet has orientation w.
///<
class MicrofacetDistribution
{
public:
    // Destructor
    virtual ~MicrofacetDistribution(){}
    // w - microfacet orientation (normal), n - surface normal
    virtual float D(float3 const& w, float3 const& n) const = 0;
    // Sample the direction accordingly to this distribution
    virtual void Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const = 0;
    // PDF of the given direction
    virtual float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;
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
    float D(float3 const& w, float3 const& n) const
    {
        float ndotw = fabs(dot(n, w));
        return (1.f / (2*PI)) * (e_ + 2) * powf(ndotw, e_);
    }
    
    // Sample the distribution
    void Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Sample halfway vector first, then reflect wi around that
        float costheta = std::powf(sample.x, 1.f / (e_ + 1.f));
        float sintheta = std::sqrt(1.f - costheta * costheta);
        
        // phi = 2*PI*ksi2
        float cosphi = std::cosf(2.f*PI*sample.y);
        float sinphi = std::sinf(2.f*PI*sample.y);
        
        // Calculate wh
        float3 wh = normalize(isect.dpdu * sintheta * cosphi + isect.dpdv * sintheta * sinphi + isect.n * costheta);
        
        // Reflect wi around wh
        wo = -wi + 2.f*dot(wi, wh) * wh;
        
        // Calc pdf
        pdf = Pdf(isect, wi, wo);
    }
    
    // PDF of the given direction
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // We need to convert pdf(wh)->pdf(wo)
        float3 wh = normalize(wi + wo);
        // costheta
        float ndotwh = dot(isect.n, wh);
        // See Humphreys and Pharr for derivation
        return ((e_ + 1.f) * std::powf(ndotwh, e_)) / (2.f * PI * 4.f * dot (wo,wh));
    }
    
    // Exponent
    float e_;
};


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
               float3 ks = float3(1.f, 1.f, 1.f),
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
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        Primitive::Intersection isectlocal = isect;
        
        // Alter normal if needed
        // TODO: fix tangents as well
        MAP_NORMAL(nmap_, isectlocal);
        
        // Revert normal if needed
        if (dot(isect.n, wi) < 0.f)
        {
            isectlocal.n = -isectlocal.n;
            isectlocal.dpdu = -isectlocal.dpdu;
            isectlocal.dpdv = -isectlocal.dpdv;
        }
        
        // Sample distribution
        md_->Sample(isectlocal, sample, wi, wo, pdf);
        
        // Evaluate
        return Evaluate(isect, wi, wo);
    }
    
    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Return 0 if wo and wi are on different sides
        float sameside = dot(wi, isect.n) * dot(wo, isect.n);
        
        if (sameside < 0.f)
            return float3(0, 0, 0);
        
        Primitive::Intersection isectlocal = isect;
        
        // Alter normal if needed
        // TODO: fix tangents as well
        MAP_NORMAL(nmap_, isectlocal);
        
        // Revert normal if needed
        float3 n = dot(isect.n, wi) < 0.f ? -isectlocal.n : isectlocal.n;
        
        // Incident and reflected zenith angles
        float cos_theta_o = dot(n, wo);
        float cos_theta_i = dot(n, wi);
        
        if (cos_theta_i == 0.f || cos_theta_o == 0.f)
            return float3(0.f, 0.f, 0.f);
        
        // Calc halfway vector
        float3 wh = normalize(wi + wo);
        
        // Calc Fresnel for wh faced microfacets
        float fresnel = fresnel_->Evaluate(1.f, eta_, dot(wi, wh));
        
        float3 ks = GET_VALUE(ks_, ksmap_, isect.uv);
        
        // F(wi,wo) = D(wh)*Fresnel(wh, n)*G(wi, wo, n)/(4 * cos_theta_i * cos_theta_o)
        return ks * (md_->D(wh, n) * G(wi, wo, wh, n) * fresnel / (4.f * cos_theta_i * cos_theta_o));
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
    
    // Return pdf for wo to be sampled for wi
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Return 0 if wo and wi are on different sides
        float sameside = dot(wi, isect.n) * dot(wo, isect.n);
        if (sameside < 0.f)
            return 0.f;
        
        Primitive::Intersection isectlocal = isect;
        
        if (dot(isectlocal.n, wi) < 0)
        {
            isectlocal.n = -isectlocal.n;
            isectlocal.dpdu = -isectlocal.dpdu;
            isectlocal.dpdv = -isectlocal.dpdv;
        }
        
        return md_->Pdf(isectlocal, wi, wo);
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