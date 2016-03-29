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
#ifndef SIMPLEMATERIAL_H
#define SIMPLEMATERIAL_H

#include <string>
#include <memory>

#include "material.h"
#include "../bsdf/bsdf.h"

///< SimpleMaterial consists of a single BSDF and routes method calls directly to it
///<
class SimpleMaterial : public Material
{
public:
    // If diffuse map is specified it is used as a diffuse color, otherwise diffuse color is used
    SimpleMaterial(Bsdf* bsdf)
    : bsdf_(bsdf)
    {
    }

    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(ShapeBundle::Hit& hit, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const
    {
        type = bsdf_->GetType();
        return bsdf_->Sample(hit, sample, wi, wo, pdf);
    }
    
    // PDF of a given direction sampled from hit.p
    float GetPdf(ShapeBundle::Hit& hit, float3 const& wi, float3 const& wo) const
    {
        return bsdf_->GetPdf(hit, wi, wo);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(ShapeBundle::Hit& hit, float3 const& wi, float3 const& wo) const
    {
        return bsdf_->Evaluate(hit, wi, wo);
    }
    
private:
    // BSDF
    std::unique_ptr<Bsdf> bsdf_;
};

#endif // SIMPLEMATERIAL_H