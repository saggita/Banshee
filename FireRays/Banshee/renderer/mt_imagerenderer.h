#ifndef MT_IMAGERENDERER_H
#define MT_IMAGERENDERER_H

class World;
class ImagePlane;
class Tracer;

#include <memory>

#include "imagerenderer.h"
#include "../math/int2.h"
#include "../async/thread_pool.h"

///< Image renderer provides the means to sample and render
///< into an imageplane instance.
///<
class MtImageRenderer : public ImageRenderer
{
public:
    // Note that imgplane is an external entity and is not managed by
    // this renderer instance. Tracer on the other hand is owned by the instance
    // and release in the destructor
    MtImageRenderer(ImagePlane& imgplane, Tracer* tracer, int2 tilesize = int2(16,16))
        : ImageRenderer(imgplane, tracer)
        , tilesize_(tilesize)
    {
    }

    void Render(World const& world) const;

private:
    int2 tilesize_;
    mutable thread_pool<int> threadpool_;
};

#endif //MT_IMAGERENDERER_H