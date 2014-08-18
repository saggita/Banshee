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
#include "primitive/mesh.h"
#include "accelerator/simpleset.h"
#include "accelerator/bvh.h"
#include "accelerator/sbvh.h"
#include "imageio/oiioimageio.h"
#include "camera/perspective_camera.h"
#include "camera/environment_camera.h"
#include "renderer/imagerenderer.h"
#include "renderer/mt_imagerenderer.h"
#include "imageplane/fileimageplane.h"
#include "tracer/ditracer.h"
#include "tracer/gitracer.h"
#include "tracer/aotracer.h"
#include "light/pointlight.h"
#include "sampler/random_sampler.h"
#include "rng/mcrng.h"
#include "material/matte.h"
#include "texture/oiio_texturesystem.h"
#include "import/assimp_assetimporter.h"
#include "util/progressreporter.h"


std::unique_ptr<World> BuildWorld(TextureSystem const& texsys)
{
    // Create world 
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    Bvh* bvh = new Bvh();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 0, 0), float3(1, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(0,-1,0), float3(0, 0, 1), float2(0.01f, 10000.f));
    
    // Create lights
    PointLight* light1 = new PointLight(float3(0.f, 1.2f, 0.f), 2.5f * float3(0.97f, 0.85f, 0.55f));

    rand_init();

    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.objm");
    AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.objm");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/crytek-sponza/sponza.objm");
    
    
    assimp.onmaterial_ = [&world](Material* mat)->int
    {
        world->materials_.push_back(std::unique_ptr<Material>(mat));
        return (int)(world->materials_.size() - 1);
    };

    std::vector<Primitive*> primitives;
    assimp.onprimitive_ = [&primitives](Primitive* prim)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        primitives.push_back(prim);
    };

    // Start assets import
    assimp.Import();
    
    // Build acceleration structure
    bvh->Build(primitives);

    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
    //world->accelerator_ = std::unique_ptr<Primitive>(set);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Attach point lights
    world->lights_.push_back(std::unique_ptr<Light>(light1));
    //world->lights_.push_back(std::unique_ptr<Light>(light2));
    // Set background
    world->bgcolor_ = float3(0.3f, 0.4f, 0.3f);

    // Return world
    return std::unique_ptr<World>(world);
}

std::unique_ptr<World> BuildWorldSponza(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    //Bvh* bvh = new Sbvh(10.f);
    Bvh* bvh = new Bvh();
    // Create camera
    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    Camera* camera = new PerscpectiveCamera(float3(-50, 0, 0), float3(1, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));

    // Create lights
    PointLight* light1 = new PointLight(float3(30.f, 1.2f, 0.f), 10000.f * float3(0.97f, 0.85f, 0.55f));

    rand_init();

    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.objm");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.objm");
    AssimpAssetImporter assimp(texsys, "../../../Resources/crytek-sponza/sponza.objm");

    assimp.onmaterial_ = [&world](Material* mat)->int
    {
        world->materials_.push_back(std::unique_ptr<Material>(mat));
        return (int)(world->materials_.size() - 1);
    };

    std::vector<Primitive*> primitives;
    assimp.onprimitive_ = [&primitives](Primitive* prim)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        primitives.push_back(prim);
    };

    // Start assets import
    assimp.Import();

    // Build acceleration structure
    auto starttime = std::chrono::high_resolution_clock::now();
    bvh->Build(primitives);
    auto endtime = std::chrono::high_resolution_clock::now();
    auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);

    std::cout << "Acceleration structure constructed in " << exectime.count() << " ms\n";

    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
    //world->accelerator_ = std::unique_ptr<Primitive>(set);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Attach point lights
    world->lights_.push_back(std::unique_ptr<Light>(light1));
    //world->lights_.push_back(std::unique_ptr<Light>(light2));
    // Set background
    world->bgcolor_ = float3(0.9f, 0.9f, 0.9f);
    
    // Return world
    return std::unique_ptr<World>(world);
}

std::unique_ptr<World> BuildWorldDragon(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    Bvh* bvh = new Sbvh(10.f);
    //Bvh* bvh = new Bvh();
    // Create camera
    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    Camera* camera = new PerscpectiveCamera(float3(1, 0, -1.1f), float3(0, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));

    // Create lights
    PointLight* light1 = new PointLight(float3(1.f, 1.f, -1.f), float3(0.97f, 0.85f, 0.55f));

    rand_init();

    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.objm");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.objm");
    AssimpAssetImporter assimp(texsys, "../../../Resources/dragon/dragon1.obj");

    assimp.onmaterial_ = [&world](Material* mat)->int
    {
        world->materials_.push_back(std::unique_ptr<Material>(mat));
        return (int)(world->materials_.size() - 1);
    };

    std::vector<Primitive*> primitives;
    assimp.onprimitive_ = [&primitives](Primitive* prim)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        primitives.push_back(prim);
    };

    // Start assets import
    assimp.Import();

    // Build acceleration structure
    auto starttime = std::chrono::high_resolution_clock::now();
    bvh->Build(primitives);
    auto endtime = std::chrono::high_resolution_clock::now();
    auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);

    std::cout << "Acceleration structure constructed in " << exectime.count() << " ms\n";

    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
    //world->accelerator_ = std::unique_ptr<Primitive>(set);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Attach point lights
    world->lights_.push_back(std::unique_ptr<Light>(light1));
    //world->lights_.push_back(std::unique_ptr<Light>(light2));
    // Set background
    world->bgcolor_ = float3(0.9f, 0.9f, 0.9f);
    
    // Return world
    return std::unique_ptr<World>(world);
}

int main()
{
    try
    {
        // File name to render
        std::string filename = "normals.png";
        int2 imgres = int2(512, 512);
        // Create texture system
        OiioTextureSystem texsys("../../../Resources/Textures");

        // Build world
        std::cout << "Constructing world...\n";
        std::unique_ptr<World> world = BuildWorldDragon(texsys);

        // Create OpenImageIO based IO api
        OiioImageIo io;
        // Create image plane writing to file
        FileImagePlane plane(filename, imgres, io);

        // Create progress reporter
        class MyReporter : public ProgressReporter
        {
        public:
            MyReporter() : prevprogress_(0)
            {
            }

            void Report(float progress)
            {
                int percents = (int)(progress * 100);

                if (percents - prevprogress_ >= 5)
                {
                    std::cout << percents << "%... ";
                    prevprogress_ = percents;
                }
            }
        private:
            int prevprogress_;
        };

        // Create renderer w/ direct illumination trace
        std::cout << "Kicking off rendering engine...\n";
        MtImageRenderer renderer(plane, // Image plane 
            new DiTracer(), // Tracer
            new RandomSampler(8, new McRng()), // Image sampler
            new RandomSampler(1, new McRng()), // Light sampler
            new MyReporter() // Progress reporter
            );

        // Measure execution time
        std::cout << "Starting rendering process...\n";
        auto starttime = std::chrono::high_resolution_clock::now();
        renderer.Render(*world);
        auto endtime = std::chrono::high_resolution_clock::now();
        auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);

        std::cout << "Rendering done\n";
        std::cout << "Image " << filename << " (" << imgres.x << "x" << imgres.y << ") rendered in " << exectime.count() / 1000.f << " s\n";
    }
    catch(std::runtime_error& e)
    {
        std::cout << e.what();
    }

    return 0;
}