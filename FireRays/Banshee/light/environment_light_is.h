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
#ifndef ENVIRONMENT_LIGHT_IS_H
#define ENVIRONMENT_LIGHT_IS_H

#include <string>
#include <memory>

class TextureSystem;

#include "light.h"

class Distribution2D;

///< EnvironmentLight represents image based light emitting from the whole sphere around.
///< The radiance values are taken from HDR lightprobe provided during construction.
///< This version uses importance sampling according to radiance values distribution,
///< so experience faster convergence than regular one.
///<
class EnvironmentLightIs: public Light
{
public:
    EnvironmentLightIs(TextureSystem const& texsys,
                       std::string const& texture,
                       float scale = 1.f,
                       float gamma = 2.2f);
    
    // Sample method generates a direction to the light(d), calculates pdf in that direction (pdf)
    // and returns radiance emitted(return value) into the direction specified by isect
    // Note that no shadow testing occurs here, the method knows nothing about world's geometry
    // and it is renderers responsibility to account for visibility term
    // [PARTIALLY DONE] TODO: account for different sampling modes here, need to pass sampler/sample?
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const;
    
    // This method is supposed to be called by the renderer when the ray misses the geometry.
    // It allows implementing IBL, etc.
    float3 Le(ray const& r) const;
    
    // PDF of a given direction sampled from isect.p
    float Pdf(Primitive::Intersection const& isect, float3 const& w) const;
    
    // Check if the light is singular (represented by delta function or not)
    bool singular() const { return false; }
    
    
private:
    // Texture system
    TextureSystem const& texsys_;
    // Texture file name
    std::string texture_;
    // Radiance scale
    float scale_;
    // Gamma
    float invgamma_;
    // Distribution for radiance
    std::unique_ptr<Distribution2D> radiancedist_;
};

#endif // ENVIRONMENT_LIGHT_H