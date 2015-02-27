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
#ifndef MIXEDMATERIAL_H
#define MIXEDMATERIAL_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/bsdf.h"
#include "../bsdf/fresnel.h"

///< MixedMaterial represents complex material combining multiple BRDFs & BTDFs
///<
class MixedMaterial : public Material
{
public:
    // Cosntructor
    MixedMaterial(float eta)
    : fresnel_(new FresnelDielectric())
    , eta_(eta)
    {
    }
    
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const
    {
        // Evaluate Fresnel and choose whether BRDFs or BTDFs should be sampled
        float reflectance = fresnel_->Evaluate(1.f, eta_, dot(isect.n, wi));
        
        float rnd = rand_float();
        
        if (btdfs_.size() == 0 || rnd < reflectance)
        {
            // Sample BRDFs
            assert(brdfs_.size());
            // Choose which one to sample
            int idx = rand_uint() % brdfs_.size();
            // Sample it
            float3 f = brdfs_[idx]->Sample(isect, sample, wi, wo, pdf);
            // Set type
            type = brdfs_[idx]->GetType();
            
            // For specular just return the value as it has implicit delta function
            if (!(type & Bsdf::SPECULAR))
            {
                // Compute PDF and value
                for (int i=0;i<(int)brdfs_.size();++i)
                {
                    if (i != idx)
                    {
                        f += brdfs_[i]->Evaluate(isect, wi, wo);
                        pdf += brdfs_[i]->Pdf(isect, wi, wo);
                        
                    }
                }
                
                // Normalize
                pdf /= brdfs_.size();
            }
            
            return f;
        }
        else
        {
            // Sample BRDFs
            assert(btdfs_.size());
            // Choose which one to sample
            int idx = rand_uint() % btdfs_.size();
            // Sample it
            float3 f = btdfs_[idx]->Sample(isect, sample, wi, wo, pdf);
            // Set type
            type = btdfs_[idx]->GetType();
            
            // For specular just return the value as it has implicit delta function
            if (!(type & Bsdf::SPECULAR))
            {
                // Compute PDF and value
                for (int i=0;i<(int)btdfs_.size();++i)
                {
                    if (i != idx)
                    {
                        f += btdfs_[i]->Evaluate(isect, wi, wo);
                        pdf += btdfs_[i]->Pdf(isect, wi, wo);
                    }
                }
                
                // Normalize
                pdf /= btdfs_.size();
            }
            
            return f;
        }
        
        
        return float3();
    }
    
    // PDF of a given direction sampled from isect.p
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Evaluate Fresnel and choose whether BRDFs or BTDFs should be sampled
        float reflectance = fresnel_->Evaluate(1.f, eta_, dot(isect.n, wi));
        
        float rnd = rand_float();
        
        float pdf = 0.f;
        
        if (btdfs_.size() == 0 || rnd < reflectance)
        {
            // Compute PDF
            for (int i=0;i<(int)brdfs_.size();++i)
            {
                pdf += brdfs_[i]->Pdf(isect, wi, wo);
            }
            
            // Normalize
            pdf /= brdfs_.size();
        }
        else
        {
            // Compute PDF
            for (int i=0;i<(int)btdfs_.size();++i)
            {
                pdf += btdfs_[i]->Pdf(isect, wi, wo);
            }
            
            // Normalize
            pdf /= btdfs_.size();
        }
        
        return pdf;
    }
    
    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        // Evaluate Fresnel and choose whether BRDFs or BTDFs should be sampled
        float reflectance = fresnel_->Evaluate(1.f, eta_, dot(isect.n, wi));
        
        float rnd = rand_float();
        
        float3 f;
        
        if (btdfs_.size() == 0 || rnd < reflectance)
        {
            // Compute PDF
            for (int i=0;i<(int)brdfs_.size();++i)
            {
                f += brdfs_[i]->Evaluate(isect, wi, wo);
            }
        }
        else
        {
            // Compute PDF
            for (int i=0;i<(int)btdfs_.size();++i)
            {
                f += btdfs_[i]->Evaluate(isect, wi, wo);
            }
        }
        
        return f;
    }
    
    // Add new BSDF to the set
    void AddBsdf(Bsdf* bsdf)
    {
        if (bsdf->GetType() & Bsdf::REFLECTION)
        {
            brdfs_.push_back(std::unique_ptr<Bsdf>(bsdf));
        }
        else
        {
            btdfs_.push_back(std::unique_ptr<Bsdf>(bsdf));
        }
    }
    
private:
    // BSDF
    std::vector<std::unique_ptr<Bsdf> > brdfs_;
    std::vector<std::unique_ptr<Bsdf> > btdfs_;
    //
    std::unique_ptr<Fresnel> fresnel_;
    // Refractive index
    float eta_;
};

#endif // MIXEDMATERIAL_H
