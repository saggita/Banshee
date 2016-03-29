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

#ifndef ADAPTIVE_RENDERER_H
#define ADAPTIVE_RENDERER_H

class World;
class ImagePlane;
class Tracer;

#include <memory>

#include "imagerenderer.h"
#include "../math/int2.h"
#include "../async/thread_pool.h"

///< Adaptive sampler is keeping track of pixel delta during the rendering
///< and reassignes the work after each pass to work more on pixels that are
///< taking longer to converge.
///<
class AdaptiveRenderer : public ImageRenderer
{
public:
    // Note that imgplane is an external entity and is not managed by
    // this renderer instance. Tracer on the other hand is owned by the instance
    // and release in the destructor
    // tilesize parameter determines granularity of a tasks assigned to cores.
    // Each task is supposed to process a single tile, change this parameter to
    // find a right balance between submission overhead and benefit from parallelization
    AdaptiveRenderer(ImagePlane& imgplane,
                     Tracer* tracer,
                     Sampler* imgsampler,
                     Sampler* lightsampler,
                     Sampler* brdfsampler,
                     int2* const pixelindices,
                     int numindices,
                     ProgressReporter* progress = nullptr,
                     int tilesize = 64)
    : ImageRenderer(imgplane, tracer, imgsampler, lightsampler, brdfsampler, progress)
    , pixelindices_(pixelindices)
    , numindices_(numindices)
    , tilesize_(tilesize)
    {
    }
    
    // Render call is always blocking
    void Render(World const& world) const;
    
    void SetIndexCount(int numindices)
    {
        numindices_ = numindices;
    }
    
private:
    // Size of a single tile aka task size
    int tilesize_;
    // Thread pool
    mutable thread_pool<int> threadpool_;
    // Adaptivity mask
    int2 const* pixelindices_;
    int numindices_;
};

#endif //ADAPTIVE_RENDERER_H
