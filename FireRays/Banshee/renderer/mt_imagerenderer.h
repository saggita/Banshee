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
        Tracer* tracer, Sampler* imgsampler, 
        Sampler* lightsampler,
        ProgressReporter* progress = nullptr,
        int2 tilesize = int2(16,16))
        : ImageRenderer(imgplane, tracer, imgsampler, lightsampler, progress)
        , tilesize_(tilesize)
    {
    }

    // Render call is always blocking 
    void Render(World const& world) const;

private:
    // Size of a single tile aka task size
    int2 tilesize_;
    // Thread pool
    mutable thread_pool<int> threadpool_;
};

#endif //MT_IMAGERENDERER_H