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

#ifndef TEXTURETRACER_H
#define TEXTURETRACER_H

#include <string>

#include "tracer.h"
#include "../primitive/shapebundle.h"

class TextureSystem;

///< TextureTracer is a debug implementation of a Tracer
///< supposed to be used in various sampler tests
class TextureTracer : public Tracer
{
public:
    // Constructor
    TextureTracer(TextureSystem const& texsys, std::string const& texture);
    
    // Estimate a radiance coming from r due to direct illumination
    float3 GetLi(ray const& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const;
    
private:
    //
    TextureSystem const& texsys_;
    // Texture name
    std::string texture_;
};

inline TextureTracer::TextureTracer(TextureSystem const& texsys, std::string const& texture)
: texsys_(texsys)
, texture_(texture)
{
}

#endif // TEXTURETRACER_H