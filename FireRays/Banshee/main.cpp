#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "math/mathutils.h"
#include "primitive/sphere.h"
#include "imageio/oiioimageio.h"
#include "camera/perspective_camera.h"


int main()
{
    try
    {
        matrix m = translation(float3(0.3, 0.3, 0));
        matrix i = m * inverse(m);
        Sphere sp(1.f, m, inverse(m));

        ray r(float3(0, 0, -10), float3(0, 0, 1), float2(0.f, 10000.f));

        float t = 10000.f;
        Primitive::Intersection isect;
        bool f = sp.Intersect(r, t, isect);

        OiioImageIo io;

        float imgdata[] = 
        {
            1,0,0,1,  0,1,0,1,
            0,0,1,1,  1,0,1,1
        };

        std::vector<float> idata;
        idata.assign(imgdata, imgdata + 16);

        io.Write("hello.png", idata, ImageIo::ImageDesc(2, 2, 4));

        PerscpectiveCamera cam(float3(0,0,-10), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
        std::vector<float3> data(256 * 256);
        std::fill(data.begin(), data.end(), float3(1,1,1));

        for (int y = 0; y < 256; ++y)
            for(int x = 0; x < 256; ++x)
            {
                ray r;
                cam.GenerateRay(float2(x / 256.f + 1.f / 512.f, (255.f - y) / 256.f + 1.f / 512.f), r);
                if (sp.Intersect(r, t, isect))
                {
                    data[y * 256 + x] = 0.5f * isect.n + float3(0.5f, 0.5f, 0.5f);
                }
            }

            std::vector<float> fdata(256*256*3);
            for (int i = 0; i < 256 * 256; ++i)
            {
                fdata[3 * i] = data[i].x;
                fdata[3 * i + 1] = data[i].y;
                fdata[3 * i + 2] = data[i].z;
            }

         io.Write("framebuffer.png", fdata, ImageIo::ImageDesc(256, 256, 3));
    }
    catch(std::runtime_error& e)
    {
        std::cout << e.what();
    }

    return 0;
}