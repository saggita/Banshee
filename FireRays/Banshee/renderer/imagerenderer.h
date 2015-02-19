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
#ifndef IMAGERENDERER_H
#define IMAGERENDERER_H

class World;
class ImagePlane;
class ProgressReporter;

#include <memory>

#include "../tracer/tracer.h"
#include "../sampler/sampler.h"
#include "renderer.h"

///< Image renderer provides the means to sample and render
///< into an imageplane instance.
///<
class ImageRenderer : public Renderer
{
public:

    // Note that imgplane is an external entity and is not managed by
    // this renderer instance. Tracer on the other hand is owned by the instance
    // and release in the destructor
    ImageRenderer(ImagePlane& imgplane, 
        Tracer* tracer, 
        Sampler* imgsampler, 
        Sampler* lightsampler,
        Sampler* brdfsampler,
        ProgressReporter* progress = nullptr
        )
        : imgplane_(imgplane)
        , tracer_(tracer)
        , imgsampler_(imgsampler)
        , lightsampler_(lightsampler)
        , brdfsampler_(brdfsampler)
        , progress_(progress)
    {
    }

    void Render(World const& world) const;

protected:
    // Image plane for an output
    ImagePlane& imgplane_;
    // Ray tracer 
    std::unique_ptr<Tracer> tracer_;
    // Sampler object to use for image plane sampling
    std::unique_ptr<Sampler> imgsampler_;
    // Sampler object to use for light sampling
    std::unique_ptr<Sampler> lightsampler_;
    // Sampler object to use for brdf sampling
    std::unique_ptr<Sampler> brdfsampler_;
    // Progress reporter
    std::unique_ptr<ProgressReporter> progress_;
};

#endif //IMAGERENDERER_H
