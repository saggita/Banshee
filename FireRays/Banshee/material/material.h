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
#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>

#include "../texture/texturesystem.h"
#include "../primitive/primitive.h"


///< Material is an interface for the renderer to call
///< when handling intersections. It encapsulates all
///< the surface appearance evaluations.
///<
class Material
{
public:
    Material ()
    {}

    // Destructor
    virtual ~Material() {}

    // Sample material and return outgoing ray direction along with combined BSDF value and sampled BSDF type
    virtual float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const = 0;

    // Evaluate combined BSDF value
    virtual float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;
    
    // PDF of a given direction sampled from isect.p
    virtual float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const = 0;

    // Indicate whether the materials has emission component and will be used for direct light evaluation
    virtual bool emissive() const { return false; }

    // Emission component of the material
    virtual float3 Le(Primitive::SampleData const& sampledata, float3 const& wo) const { return float3(0,0,0); }
};
#endif // MATERIAL_H