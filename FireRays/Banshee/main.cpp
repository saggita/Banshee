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
#include "primitive/indexed_triangle.h"
#include "accelerator/simpleset.h"
#include "imageio/oiioimageio.h"
#include "camera/perspective_camera.h"
#include "renderer/imagerenderer.h"
#include "renderer/mt_imagerenderer.h"
#include "imageplane/fileimageplane.h"
#include "tracer/ditracer.h"
#include "light/pointlight.h"
#include "sampler/random_sampler.h"
#include "rng/mcrng.h"


std::unique_ptr<World> BuildWorld()
{
    // Create world 
    World* world = new World();
    // Create accelerator
    SimpleSet* set = new SimpleSet();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(0, 0,-10), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 480.f/360.f);
    // Create lights
    PointLight* light1 = new PointLight(float3(5, 15, -2), float3(0.2f, 0.2f, 0.2f));
    PointLight* light2 = new PointLight(float3(-5, 5, -2), float3(0.99f, 0.9f, 0.74f));

    rand_init();

    // Generate 10 spheres in [-1.5, 1.5] cube
    for (int i = 0; i < 10; ++i)
    {
        matrix worldmat = translation(float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f));

        Sphere* sphere = new Sphere(rand_float() * 0.5f, worldmat, inverse(worldmat));
        set->Emplace(sphere);
    }

    // Add ground plane
    // TODO: leak here as triangles are designed to be owned by meshes
    float3* vertices = new float3[4];
    vertices[0] = float3(-500, -2, -500);
    vertices[1] = float3(-500, -2, 500);
    vertices[2] = float3(500, -2, 500);
    vertices[3] = float3(500, -2, -500);

    float3* normals = new float3[4];
    normals[0] = float3(0, 1, 0);
    normals[1] = float3(0, 1, 0);
    normals[2] = float3(0, 1, 0);
    normals[3] = float3(0, 1, 0);

    float2* uvs = new float2[4];
    uvs[0] = float2(0, 0);
    uvs[1] = float2(0, 1);
    uvs[2] = float2(1, 1);
    uvs[3] = float2(1, 0);

    IndexedTriangle* t1 = new IndexedTriangle(0, 3, 1, 0, 3, 1, 0, 3, 1, 0, vertices, normals, uvs); 
    IndexedTriangle* t2 = new IndexedTriangle(3, 1, 2, 3, 1, 2, 3, 1, 2, 0, vertices, normals, uvs);

    set->Emplace(t1);
    set->Emplace(t2);

    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(set);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Attach point lights
    world->lights_.push_back(std::unique_ptr<Light>(light1));
    world->lights_.push_back(std::unique_ptr<Light>(light2));
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
        int2 imgres = int2(480, 360);

        // Build world
        std::unique_ptr<World> world = BuildWorld();

        // Create OpenImageIO based IO api
        OiioImageIo io;
        // Create image plane writing to file
        FileImagePlane plane(filename, imgres, io);
        // Create renderer w/ direct illumination tracer
        ImageRenderer renderer(plane, new DiTracer(), new RandomSampler(32, new McRng()));

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