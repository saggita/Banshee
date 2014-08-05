#include "mt_imagerenderer.h"

#include "../world/world.h"
#include "../imageplane/imageplane.h"
#include "../tracer/tracer.h"


#include <cassert>
#include <vector>
#include <algorithm>

void MtImageRenderer::Render(World const& world) const
{
    int2 imgres = imgplane_.resolution();

    // Get camera 
    Camera const& cam(*world.camera_.get());

    // Prepare image plane
    imgplane_.Prepare();

    // Calculate the number of tiles
    int2 numtiles = int2((imgres.x + tilesize_.x - 1) / tilesize_.x, (imgres.y + tilesize_.y - 1) / tilesize_.y);

    //Futures to wait
    std::vector<std::future<int> > futures;

    for (int xtile = 0; xtile < numtiles.x; ++xtile)
        for (int ytile = 0; ytile < numtiles.y; ++ytile)
        {
            futures.push_back(
                threadpool_.submit([&, xtile, ytile, imgres]()->int
            {
                for (int x = 0; x < tilesize_.x; ++x)
                    for (int y = 0; y < tilesize_.y;++y)
                    {
                        int xx = xtile * tilesize_.x + x;
                        int yy = ytile * tilesize_.y + y;

                        if (xx >= imgres.x || yy >= imgres.y)
                            return 0;

                        ray r;

                        float2 sample((float)xx / imgres.x + 1.f / (imgres.x*2), (float)yy / imgres.y + 1.f / (imgres.y*2));

                        // Generate ray
                        cam.GenerateRay(sample, r);

                        // Estimate radiance and add to image plane
                        imgplane_.AddSample(sample, 1.f, tracer_->Li(r, world));
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