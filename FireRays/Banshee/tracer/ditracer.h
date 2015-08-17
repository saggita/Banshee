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

#ifndef DITRACER_H
#define DITRACER_H

#include "tracer.h"
#include "../primitive/shapebundle.h"

class Light;
class Sampler;

///< DiTracer is an implementation of a Tracer interface capable of estimating only direct illumination from surfaces. This implementation uses
///< multiple-importance sampling to reduce variance: https://graphics.stanford.edu/courses/cs348b-03/papers/veach-chapter9.pdf
///<
class DiTracer : public Tracer
{
public:
    // Constructor
    DiTracer(){}

    // Estimate a radiance coming from r due to direct illumination
    float3 GetLi(ray const& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const;

protected:
    // Estimate direct illimination component due to light contribution reflected along wo
    virtual float3 GetDi(World const& world, Light const& light, Sampler const& lightsampler, Sampler const& bsdfsampler, float3 const& wo, ShapeBundle::Hit& hit) const;
};

#endif // DITRACER_H