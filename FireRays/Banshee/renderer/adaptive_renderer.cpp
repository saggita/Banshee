#include "adaptive_renderer.h"

#include "../world/world.h"
#include "../imageplane/imageplane.h"
#include "../tracer/tracer.h"
#include "../util/progressreporter.h"
#include "../math/mathutils.h"


#include <cassert>
#include <vector>
#include <algorithm>
#include <mutex>
#include <atomic>

void AdaptiveRenderer::Render(World const& world) const
{
    // Image resolution
    int2 imgres = imgplane_.resolution();
    
    // Get camera
    Camera const& cam(*world.camera_.get());
    
    // Prepare image plane
    imgplane_.Prepare();
    
    // Calculate the number of tiles to handle
    int numtiles = (numindices_ + tilesize_ - 1) / tilesize_;
    
    // Futures to wait
    std::vector<std::future<int> > futures;
    
    // Prepare count and mutex for progress reporting
    std::mutex progressmutex;
    int totalsamples = imgsampler_->num_samples() * numindices_;
    int donesamples = 0;
    
    // Iterate over all the tiles
    // Note that Sampler objects are not thread safe
    // and not designed for concurrent access.
    // We are cloning the sampler for each task instead.
    //
    for (int xtile = 0; xtile < numtiles; ++xtile)
    {
        // Clone the samplers first
        Sampler* imgsampler = imgsampler_->Clone();
        Sampler* lightsampler = lightsampler_->Clone();
        Sampler* brdfsampler = brdfsampler_->Clone();
        
        // Submit the task to thread pool
        // Need to capture xtile and ytile by copying since
        // they are changing, same for sampler objects
        futures.push_back(
                          threadpool_.submit([&, xtile, imgsampler, lightsampler, brdfsampler]()->int
                                             {
                                                 // Wrap private samplers w/ memory managing ptr
                                                 std::unique_ptr<Sampler> private_imgsampler(imgsampler);
                                                 std::unique_ptr<Sampler> private_lightsampler(lightsampler);
                                                 std::unique_ptr<Sampler> private_brdfsampler(brdfsampler);
                                                 // Iterate through tile pixels
                                                 for (int x = 0; x < tilesize_; ++x)
                                                     {
                                                         // Calculate pixel index
                                                         int xx = xtile * tilesize_ + x;
                                                         
                                                         if (xx >= numindices_)
                                                         {
                                                             return 0;
                                                         }
                                                         
                                                         int2 p = pixelindices_[xx];
                                                         
                                                         ray r;
                                                         
                                                         float sample_weight = 1.f / private_imgsampler->num_samples();
                                                         
                                                         for (int s = 0; s < private_imgsampler->num_samples(); ++s)
                                                         {
                                                             // Generate sample
                                                             float2 sample = private_imgsampler->Sample2D();
                                                             
                                                             // Calculate image plane sample
                                                             float2 imgsample((float)p.x / imgres.x + (1.f / imgres.x) * sample.x, (float)p.y / imgres.y + (1.f / imgres.y) * sample.y);
                                                             
                                                             // Generate ray
                                                             cam.GenerateRay(imgsample, r);
                                                             
                                                             // Estimate radiance and add to image plane
                                                             imgplane_.AddSample(p, sample_weight, tracer_->Li(r, world, *private_lightsampler, *private_brdfsampler));
                                                         }
                                                     }
                                                 
                                                 // Update and report progress
                                                 if (progress_)
                                                 {
                                                     std::unique_lock<std::mutex> lock(progressmutex);
                                                     
                                                     donesamples += tilesize_ * private_imgsampler->num_samples();
                                                     
                                                     progress_->Report((float)donesamples / totalsamples);
                                                 }
                                                 
                                                 return 0;
                                             })
                          );
    }
    
    // Not sure if this is needed, we seem to bail out on Win 7 anyway
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    std::for_each(futures.begin(), futures.end(), std::mem_fun_ref(&std::future<int>::wait));
    
    // Finalize image plane
    imgplane_.Finalize();
}