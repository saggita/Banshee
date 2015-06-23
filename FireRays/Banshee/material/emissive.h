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
#ifndef EMISSIVE_H
#define EMISSIVE_H

#include "material.h"


///< Emissive material adds illumination characteristic to the object.
///< This material needs special treatment in order to accurately
///< compute the radiance for the scene. Not only we need to add its
///< Le contribution to the ray hitting emissive object, but also
///< use these objects for direct illumination computations.
///<
class Emissive : public Material
{
public:
    Emissive (float3 const& e)
        : e_(e)
    {
    }


    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const
    {
        // Make sure to set PDF to 0, method is not supposed to be called 
        pdf = 0.f;

        // This method is not supposed to be called on emissive, but anyway
        return Le(isect, wi);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection& isect, float3 const& wi, float3 const& wo) const
    {
        // This method is not supposed to be called on emissive, but anyway
        return Le(isect, wi);
    }
    
    // PDF of a given direction sampled from isect.p
    float Pdf(Primitive::Intersection& isect, float3 const& wi, float3 const& wo) const
    {
        return 0.f;
    }

    // Indicate whether the materials has emission component and will be used for direct light evaluation
    bool emissive() const { return true; }

    // Emission component of the material
    float3 Le(Primitive::SampleData const& sampledata, float3 const& wo) const 
    { 
        // Cosine term
        float  ndotwo = dot(sampledata.n, wo);

        if (ndotwo > 0.f)
        {
            // Scale by cosine term
            return e_ * ndotwo;
        }

        return float3(0,0,0);
    }

private:
    // Emissive power
    float3 e_;

};



#endif // EMISSIVE_H