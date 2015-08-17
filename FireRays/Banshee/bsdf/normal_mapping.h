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
#ifndef NORMALMAPPING_H
#define NORMALMAPPING_H

#include "bsdf.h"

///< Decorator enclsoing real BSDF which adds normal mapping capabilities
///<
class NormalMapping : public Bsdf
{
public:
    // Constructor
    NormalMapping(
                  // Enclosed BSDF
                  Bsdf* bsdf,
                  // Normal map
                  std::string nmap
                  )
    : Bsdf(bsdf->GetTextureSystem(), bsdf->GetType())
    , bsdf_(bsdf)
    , nmap_(nmap)
    {
    }
    
    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(ShapeBundle::Hit& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Alter normal if needed
        MAP_NORMAL(nmap_, isect);
        
        return bsdf_->Sample(isect, sample, wi, wo, pdf);
    }
    
    // Evaluate combined BSDF value
    float3 Evaluate(ShapeBundle::Hit& isect, float3 const& wi, float3 const& wo) const
    {
        // Alter normal if needed
        MAP_NORMAL(nmap_, isect);
        
        return bsdf_->Evaluate(isect, wi, wo);
    }
    
    // PDF of a given direction sampled from isect.p
    float GetPdf(ShapeBundle::Hit& isect, float3 const& wi, float3 const& wo) const
    {
        ShapeBundle::Hit isectlocal = isect;
        // Alter normal if needed
        MAP_NORMAL(nmap_, isectlocal);
        
        return bsdf_->GetPdf(isectlocal, wi, wo);
    }
    
    // Get BSDF type
    int GetType() const { return bsdf_->GetType(); }
    
protected:
    Bsdf* bsdf_;
    //
    std::string nmap_;
    
};


#endif // NORMALMAPPING_H