#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "math/mathutils.h"
#include "world/custom_worldbuilder.h"
#include "world/world.h"
#include "primitive/sphere.h"
#include "accelerator/simpleset.h"
#include "imageio/oiioimageio.h"
#include "camera/perspective_camera.h"
#include "renderer/imagerenderer.h"
#include "imageplane/fileimageplane.h"
#include "tracer/ditracer.h"
#include "light/pointlight.h"


std::unique_ptr<World> BuildWorld()
{
    // Create world 
    World* world = new World();
    // Create accelerator
    SimpleSet* set = new SimpleSet();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(0,0,-10), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    // Create light
    PointLight* light = new PointLight(float3(5, 15, -2), float3(0.6f, 0.7f, 0.9f));

    rand_init();

    // Generate 100 spheres in [-1.5, 1.5] cube
    for (int i = 0; i < 100; ++i)
    {
        matrix worldmat = translation(float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f));

        Sphere* sphere = new Sphere(rand_float() * 0.5f, worldmat, inverse(worldmat));
        set->Emplace(sphere);
    }

    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(set);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Attach point light
    world->lights_.push_back(std::unique_ptr<Light>(light));
    // Set background
    world->bgcolor_ = float3(0.3f, 0.4f, 0.3f);

    // Return world
    return std::unique_ptr<World>(world);
}

int main()
{
    try
    {
        // File name to render
        std::string filename = "normals.png";
        int2 imgres = int2(1024, 1024);

        // Build world
        std::unique_ptr<World> world = BuildWorld();

        // Create OpenImageIO based IO api
        OiioImageIo io;
        // Create image plane writing to file
        FileImagePlane plane(filename, imgres, io);
        // Create renderer w/ direct illumination tracer
        ImageRenderer renderer(plane, new DiTracer());

        // Measure execution time
        auto starttime = std::chrono::high_resolution_clock::now();
        renderer.Render(*world);
        auto endtime = std::chrono::high_resolution_clock::now();
        auto exectime = std::chrono::duration_cast<std::chrono::seconds>(endtime - starttime);

        std::cout << "Image " << filename << " (" << imgres.x << "x" << imgres.y << ") rendered in " << exectime.count() << " seconds\n";
    }
    catch(std::runtime_error& e)
    {
        std::cout << e.what();
    }

    return 0;
}