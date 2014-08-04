#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "math/mathutils.h"
#include "world/world.h"
#include "primitive/sphere.h"
#include "accelerator/simpleset.h"
#include "imageio/oiioimageio.h"
#include "camera/perspective_camera.h"


int main()
{
    try
    {
        matrix m = translation(float3(0.3, 0.3, 0));
        matrix i = m * inverse(m);
        Sphere sp(1.f, m, inverse(m));
        
        rand_init();
        
        World world;
        SimpleSet* set = new SimpleSet();
        
        for (int i = 0; i < 100; ++i)
        {
            matrix worldmat = translation(float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f));
            
            Sphere* sphere = new Sphere(rand_float() * 0.5f, worldmat, inverse(worldmat));
            set->Emplace(sphere);
        }
        
        world.accelerator_ = std::unique_ptr<Primitive>(set);
        
        OiioImageIo io;

        int const RES = 512;
        PerscpectiveCamera cam(float3(0,0,-10), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
        std::vector<float3> data(RES * RES);
        std::fill(data.begin(), data.end(), float3(1,1,1));

        Primitive::Intersection isect;
        float t;
        for (int y = 0; y < RES; ++y)
            for(int x = 0; x < RES; ++x)
            {
                ray r;
                cam.GenerateRay(float2((float)x / RES + 1.f / (RES*2), (float)(RES - 1.f - y) / RES + 1.f / (RES*2)), r);
                if (world.Intersect(r, t, isect))
                {
                    data[y * RES + x] = 0.5f * isect.n + float3(0.5f, 0.5f, 0.5f);
                }
            }

            std::vector<float> fdata(RES*RES*3);
            for (int i = 0; i < RES * RES; ++i)
            {
                fdata[3 * i] = data[i].x;
                fdata[3 * i + 1] = data[i].y;
                fdata[3 * i + 2] = data[i].z;
            }

         io.Write("framebuffer.png", fdata, ImageIo::ImageDesc(RES, RES, 3));
    }
    catch(std::runtime_error& e)
    {
        std::cout << e.what();
    }

    return 0;
}