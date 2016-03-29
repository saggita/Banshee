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

#ifndef MT_IMAGERENDERER_H
#define MT_IMAGERENDERER_H

class World;
class ImagePlane;
class Tracer;

#include <memory>

#include "imagerenderer.h"
#include "../math/int2.h"
#include "../async/thread_pool.h"

///< MtImageRenderer is a variant of ImageRenderer 
///< making use of multithreading with a thread_pool
///<
class MtImageRenderer : public ImageRenderer
{
public:
    // Note that imgplane is an external entity and is not managed by
    // this renderer instance. Tracer on the other hand is owned by the instance
    // and release in the destructor
    // tilesize parameter determines granularity of a tasks assigned to cores.
    // Each task is supposed to process a single tile, change this parameter to
    // find a right balance between submission overhead and benefit from parallelization
    MtImageRenderer(ImagePlane& imgplane, 
        Tracer* tracer,
        Sampler* imgsampler,
        Sampler* lightsampler,
        Sampler* brdfsampler,
        ProgressReporter* progress = nullptr,
        int2 tilesize = int2(16,16))
        : ImageRenderer(imgplane, tracer, imgsampler, lightsampler, brdfsampler, progress)
        , tilesize_(tilesize)
    {
    }

    // Render call is always blocking 
    void Render(World const& world) const;

    void RenderTile(World const& world, int2 const& start, int2 const& dim) const;

private:
    // Size of a single tile aka task size
    int2 tilesize_;
    // Thread pool
    mutable thread_pool<int> threadpool_;
};

#endif //MT_IMAGERENDERER_H