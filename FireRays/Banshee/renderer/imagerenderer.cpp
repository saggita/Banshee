#include "imagerenderer.h"

#include "../world/world.h"
#include "../imageplane/imageplane.h"
#include "../util/progressreporter.h"

#include <cassert>

void ImageRenderer::Render(World const& world) const
{
    int2 imgres = imgplane_.resolution();

    // Get camera 
    Camera const& cam(*world.camera_.get());

    // Prepare image plane
    imgplane_.Prepare();

    // Calculate total number of samples for progress reporting
    int totalsamples = imgsampler_->num_samples() * imgres.y * imgres.x;
    int donesamples = 0;

    // very simple render loop
    for (int y = 0; y < imgres.y; ++y)
        for(int x = 0; x < imgres.x; ++x)
        {
            ray r;

            float sample_weight = 1.f / imgsampler_->num_samples();

            for (int s = 0; s < imgsampler_->num_samples(); ++s)
            {
                // Generate sample
                float2 sample = imgsampler_->Sample2D();

                // Calculate image plane sample
                float2 imgsample((float)x / imgres.x + (1.f / imgres.x) * sample.x, (float)y / imgres.y + (1.f / imgres.y) * sample.y);

                // Generate ray
                cam.GenerateRay(imgsample, r);

                // Estimate radiance and add to image plane
                imgplane_.AddSample(imgsample, sample_weight, tracer_->Li(r, world, *lightsampler_, *brdfsampler_));
            }

            // Update progress
            donesamples += imgsampler_->num_samples();
            // Report progress
            if (progress_)
            {
                progress_->Report((float)donesamples/totalsamples);
            }
        }

    // Finalize image plane
    imgplane_.Finalize();
}