#ifndef IMAGERENDERER_H
#define IMAGERENDERER_H

class World;
class ImagePlane;

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
    ImageRenderer(ImagePlane& imgplane, Tracer* tracer, Sampler* imgsampler, Sampler* lightsampler)
        : imgplane_(imgplane)
        , tracer_(tracer)
        , imgsampler_(imgsampler)
        , lightsampler_(lightsampler)
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
};

#endif //IMAGERENDERER_H
