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
#ifndef GLASS_H
#define GLASS_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/perfect_reflect.h"
#include "../bsdf/perfect_refract.h"

///< Glass is custom material designed to represent reflective + refractive surface
///<
class Glass : public Material
{
public:
    // Constructor
    Glass(// Texture system
          TextureSystem const& texsys,
          // Refractive index
          float eta,
          // Specular color
          float3 const& ks,
          // Specular texture
          std::string const& ksmap = ""
          )
    : brdf_(new PerfectReflect(texsys, eta, ks, ksmap, "", new FresnelDielectric()))
    , btdf_(new PerfectRefract(texsys, eta, ks, ksmap, "", new FresnelDielectric()))
    , fresnel_(new FresnelDielectric())
    , eta_(eta)
    {
    }
    
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const
    {
        // Split sampling based on Fresnel
        float rnd = rand_float();
        
        float r = fresnel_->Evaluate(1.f, eta_, dot(isect.n, wi));
        
        // Reflective part
        if (rnd < r)
        {
            type = brdf_->GetType();
            float3 f = brdf_->Sample(isect, sample, wi, wo, pdf);
            pdf *= r;
            return f;
        }
        // Refractive part
        else
        {
            type = btdf_->GetType();
            float3 f = btdf_->Sample(isect, sample, wi, wo, pdf);
            pdf *= (1.f - r);
            return f;
        }
    }
    
    // PDF of a given direction sampled from isect.p
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        return 0.f;
    }
    
    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        return float3();
    }
    
private:
    // Refractive index
    float eta_;
    std::unique_ptr<Bsdf> brdf_;
    std::unique_ptr<Bsdf> btdf_;
    // TODO: need to save space and not spawn Fresnel objects everywhere
    std::unique_ptr<Fresnel> fresnel_;
 };

#endif // GLASS_H
