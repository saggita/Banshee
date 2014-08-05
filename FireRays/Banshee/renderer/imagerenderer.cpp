#include "imagerenderer.h"

#include "../world/world.h"
#include "../imageplane/imageplane.h"
#include "../tracer/tracer.h"

#include <cassert>

void ImageRenderer::Render(World const& world) const
{
    int2 imgres = imgplane_.resolution();

    // Get camera 
    Camera const& cam(*world.camera_.get());

    // Prepare image plane
    imgplane_.Prepare();

    // very simple render loop)
    float t;
    for (int y = 0; y < imgres.y; ++y)
        for(int x = 0; x < imgres.x; ++x)
        {
            ray r;

            // Calculate image plane sample
            float2 sample((float)x / imgres.x + 1.f / (imgres.x*2), (float)y / imgres.y + 1.f / (imgres.y*2));

            // Generate ray
            cam.GenerateRay(sample, r);

            // Estimate radiance and add to image plane
            imgplane_.AddSample(sample, 1.f, tracer_->Li(r, world));
        }

    // Finalize image plane
    imgplane_.Finalize();
}