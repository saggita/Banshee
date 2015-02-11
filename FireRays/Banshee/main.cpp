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
#include "accelerator/hlbvh.h"
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
#include "light/directional_light.h"
#include "light/environment_light.h"
#include "light/arealight.h"
#include "sampler/random_sampler.h"
#include "sampler/regular_sampler.h"
#include "sampler/stratified_sampler.h"
#include "rng/mcrng.h"
#include "material/simplematerial.h"
#include "material/emissive.h"
#include "bsdf/lambert.h"
#include "bsdf/microfacet.h"
#include "bsdf/perfect_reflect.h"
#include "bsdf/perfect_refract.h"
#include "texture/oiio_texturesystem.h"
#include "import/assimp_assetimporter.h"
#include "util/progressreporter.h"


//std::unique_ptr<World> BuildWorld(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    //SimpleSet* set = new SimpleSet();
//    Bvh* bvh = new Sbvh(10.f, 8);
//    // Create camera
//    Camera* camera = new PerscpectiveCamera(float3(0, 1.0f, 4.5), float3(0, 1.0f, 0), float3(0, 1.f, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    //Camera* camera = new PerscpectiveCamera(float3(0, 0, 0), float3(1, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
//    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(0,-1,0), float3(0, 0, 1), float2(0.01f, 10000.f));
//
//    rand_init();
//
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
//    AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/crytek-sponza/sponza.obj");
//
//    assimp.onmaterial_ = [&world](Material* mat)->int
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(mat));
//        return (int)(world->materials_.size() - 1);
//    };
//
//    std::vector<Primitive*> primitives;
//    assimp.onprimitive_ = [&primitives](Primitive* prim)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        primitives.push_back(prim);
//    };
//
//    assimp.onlight_ = [&world](Light* light)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        world->lights_.push_back(std::unique_ptr<Light>(light));
//    };
//
//    // Start assets import
//    assimp.Import();
//
//    // Build acceleration structure
//    bvh->Build(primitives);
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    //world->accelerator_ = std::unique_ptr<Primitive>(set);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Set background
//    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//
//std::unique_ptr<World> BuildWorldHairball(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    //SimpleSet* set = new SimpleSet();
//    Bvh* bvh = new Sbvh(10.f, 8, true, 20, 0.001f);
//    // Create camera
//    Camera* camera = new PerscpectiveCamera(float3(0, 15.f, 15.f), float3(0, 0.f, 0), float3(0, 1.f, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    //Camera* camera = new PerscpectiveCamera(float3(0, 0, 0), float3(1, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
//    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(0,-1,0), float3(0, 0, 1), float2(0.01f, 10000.f));
//
//    rand_init();
//
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
//    AssimpAssetImporter assimp(texsys, "../../../Resources/hairball/hairball.obj");
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/crytek-sponza/sponza.obj");
//
//    assimp.onmaterial_ = [&world](Material* mat)->int
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(mat));
//        return (int)(world->materials_.size() - 1);
//    };
//
//    std::vector<Primitive*> primitives;
//    assimp.onprimitive_ = [&primitives](Primitive* prim)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        primitives.push_back(prim);
//    };
//
//    assimp.onlight_ = [&world](Light* light)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        world->lights_.push_back(std::unique_ptr<Light>(light));
//    };
//
//    // Start assets import
//    assimp.Import();
//
//    // Build acceleration structure
//    bvh->Build(primitives);
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    //world->accelerator_ = std::unique_ptr<Primitive>(set);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Set background
//    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
//
//    EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.6f);
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//std::unique_ptr<World> BuildWorldSponza(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    //SimpleSet* set = new SimpleSet();
//    Bvh* bvh = new Sbvh(1.f, 8, true, 10, 0.0001f);
//    //Bvh* bvh = new Bvh();
//    // Create camera
//    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    Camera* camera = new PerscpectiveCamera(float3(-350, 650.f, 0), float3(1, 550.f, 0), float3(0, 1, 0), float2(0.005f, 10000.f), PI / 3, 1.f);
//    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));
//
//    // Create lights
//    DirectionalLight* light1 = new DirectionalLight(float3(-0.1f, -1.f, -0.1f), 2.f * float3(0.97f, 0.85f, 0.55f));
//
//    rand_init();
//
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
//    AssimpAssetImporter assimp(texsys, "../../../Resources/crytek-sponza/sponza.obj");
//
//    assimp.onmaterial_ = [&world](Material* mat)->int
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(mat));
//        return (int)(world->materials_.size() - 1);
//    };
//
//    std::vector<Primitive*> primitives;
//    assimp.onprimitive_ = [&primitives](Primitive* prim)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        primitives.push_back(prim);
//    };
//
//    // Start assets import
//    assimp.Import();
//
//    // Build acceleration structure
//    auto starttime = std::chrono::high_resolution_clock::now();
//    bvh->Build(primitives);
//    auto endtime = std::chrono::high_resolution_clock::now();
//    auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);
//
//    std::cout << "Acceleration structure constructed in " << exectime.count() << " ms\n";
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    //world->accelerator_ = std::unique_ptr<Primitive>(set);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    //world->lights_.push_back(std::unique_ptr<Light>(light2));
//    // Set background
//    world->bgcolor_ = float3(0.f, 0.f, 0.f);
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//
//std::unique_ptr<World> BuildWorldSibenik(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    //SimpleSet* set = new SimpleSet();
//    Bvh* bvh = new Sbvh(10.f, 4, true, 10, 0.00001f);
//    //Bvh* bvh = new Bvh();
//    // Create camera
//    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    Camera* camera = new PerscpectiveCamera(float3(-16.f, -13.f, 0), float3(1, -15.f, 0), float3(0, 1, 0), float2(0.005f, 10000.f), PI / 3, 1.f);
//    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));
//
//    // Create lights
//    PointLight* light1 = new PointLight(float3(-3.f, 0.f, 0.f), 100.f * float3(0.97f, 0.85f, 0.55f));
//
//    rand_init();
//
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
//    AssimpAssetImporter assimp(texsys, "../../../Resources/sibenik/sibenik.obj");
//
//    assimp.onmaterial_ = [&world](Material* mat)->int
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(mat));
//        return (int)(world->materials_.size() - 1);
//    };
//
//    std::vector<Primitive*> primitives;
//    assimp.onprimitive_ = [&primitives](Primitive* prim)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        primitives.push_back(prim);
//    };
//
//    // Start assets import
//    assimp.Import();
//
//    // Build acceleration structure
//    auto starttime = std::chrono::high_resolution_clock::now();
//    bvh->Build(primitives);
//    auto endtime = std::chrono::high_resolution_clock::now();
//    auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);
//
//    std::cout << "Acceleration structure constructed in " << exectime.count() << " ms\n";
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    //world->accelerator_ = std::unique_ptr<Primitive>(set);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    //world->lights_.push_back(std::unique_ptr<Light>(light2));
//    // Set background
//    world->bgcolor_ = float3(0.1f, 0.1f, 0.1f);
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//
//std::unique_ptr<World> BuildWorldMuseum(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    //SimpleSet* set = new SimpleSet();
//    Bvh* bvh = new Sbvh(1.f, 8, true, 10, 0.0001f);
//    //Bvh* bvh = new Bvh();
//    // Create camera
//    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    Camera* camera = new PerscpectiveCamera(float3(-2.f, -4.f, -13.f), float3(0, -4.f, -13.f), float3(0, 1, 0), float2(0.0025f, 10000.f), PI / 3, 1.f);
//    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));
//
//    // Create lights
//    DirectionalLight* light1 = new DirectionalLight(float3(0.25f, -1.f, -1.f), 200.f * float3(0.97f, 0.85f, 0.55f));
//    PointLight* light2 = new PointLight(float3(6.f, -4.f, -9.f), 200.f * float3(0.97f, 0.85f, 0.55f));
//
//    rand_init();
//
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
//    AssimpAssetImporter assimp(texsys, "../../../Resources/contest/museumhallRD.obj");
//
//    assimp.onmaterial_ = [&world](Material* mat)->int
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(mat));
//        return (int)(world->materials_.size() - 1);
//    };
//
//    std::vector<Primitive*> primitives;
//    assimp.onprimitive_ = [&primitives](Primitive* prim)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        primitives.push_back(prim);
//    };
//
//    // Start assets import
//    assimp.Import();
//
//    // Build acceleration structure
//    auto starttime = std::chrono::high_resolution_clock::now();
//    bvh->Build(primitives);
//    auto endtime = std::chrono::high_resolution_clock::now();
//    auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);
//
//    std::cout << "Acceleration structure constructed in " << exectime.count() << " ms\n";
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    //world->accelerator_ = std::unique_ptr<Primitive>(set);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    world->lights_.push_back(std::unique_ptr<Light>(light2));
//    // Set background
//    world->bgcolor_ = float3(0.1f, 0.1f, 0.1f);
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//
std::unique_ptr<World> BuildWorldDragon(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    Bvh* bvh = new Hlbvh(); //Sbvh(10.f, 8, true, 10, 0.001f);
    //Bvh* bvh = new Bvh();
    // Create camera
    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    Camera* camera = new PerscpectiveCamera(float3(1, 0, -1.1f), float3(0, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));

    // Create lights
    PointLight* light1 = new PointLight(float3(1.f, 1.f, -1.f), float3(0.97f, 0.85f, 0.55f));

    rand_init();

    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
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

    // Add ground plane
    float3 vertices[4] = {
        float3(-1, 0, -1),
        float3(-1, 0, 1),
        float3(1, 0, 1),
        float3(1, 0, -1)
    };

    float3 normals[4] = {
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 1, 0)
    };

    float2 uvs[4] = {
        float2(0, 0),
        float2(0, 1),
        float2(1, 1),
        float2(1, 0)
    };

    int indices[6] = {
        0, 3, 1,
        3, 1, 2
    };

    // Clear default material
    world->materials_.clear();

    //world->materials_.push_back(std::unique_ptr<Material>(new Refract(texsys, 2.3f, float3(0.9f, 0.3f, 0.0f))));
    world->materials_.push_back(std::unique_ptr<Material>(new SimpleMaterial(
                                                          new Lambert(texsys, float3(0.4f, 0.4f, 0.2f)))));
    
    world->materials_.push_back(std::unique_ptr<Material>(new SimpleMaterial(
                                                                             new Microfacet(texsys, 5.f, float3(1.f, 1.f, 1.f), "", "", new FresnelDielectric(), new BlinnDistribution(300.f)))));

    int materials[2] = {1, 1};

    matrix worldmat = translation(float3(0, -0.28f, 0)) * scale(float3(5, 1, 5));

    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
                          &normals[0].x, 4, sizeof(float3),
                          &uvs[0].x, 4, sizeof(float2),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          materials, sizeof(int),
                          2, worldmat, inverse(worldmat));

    primitives.push_back(mesh);

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
    world->bgcolor_ = float3(0.4f, 0.4f, 0.4f);

    // Return world
    return std::unique_ptr<World>(world);
}


//std::unique_ptr<World> BuildWorldTest(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    Bvh* bvh = new Sbvh(10.f, 8);
//    // Create camera
//    Camera* camera = new PerscpectiveCamera(float3(2, 4.3f, -13.5f), float3(2.f,0.0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    // Create lights
//    //PointLight* light1 = new PointLight(float3(0, 5, 5), 30.f * float3(3.f, 3.f, 3.f));
//    EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.6f);
//    //PointLight* light2 = new PointLight(float3(-5, 5, -2), 0.6 * float3(3.f, 2.9f, 2.4f));
//
//     // Add ground plane
//    float3 vertices[4] = {
//        float3(-5, 0, -5),
//        float3(-5, 0, 5),
//        float3(5, 0, 5),
//        float3(5, 0, -5)
//    };
//
//    float3 normals[4] = {
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0)
//    };
//
//    float2 uvs[4] = {
//        float2(0, 0),
//        float2(0, 1),
//        float2(1, 1),
//        float2(1, 0)
//    };
//
//    int indices[6] = {
//        0, 3, 1,
//        3, 1, 2
//    };
//
//    int materials[2] = {0,0};
//
//    matrix worldmat = translation(float3(0, -1.f, 0)) * scale(float3(5, 1, 5));
//
//    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
//                          &normals[0].x, 4, sizeof(float3),
//                          &uvs[0].x, 4, sizeof(float2),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          materials, sizeof(int),
//                          2, worldmat, inverse(worldmat));
//
//    std::vector<Primitive*> prims;
//    prims.push_back(mesh);
//
//    worldmat = translation(float3(-2, 0, 0)) * rotation_x(PI/2);
//    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));
//
//    worldmat = translation(float3(2, 0, 0));
//    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 2));
//
//    worldmat = translation(float3(6, 0, 0));
//    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 3));
//
//    bvh->Build(prims);
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    // Set background
//    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
//
//    // Build materials
//
//    Matte* matte0 = new Matte(texsys, float3(0.2f, 0.6f, 0.2f));
//    Matte* matte1 = new Matte(texsys, float3(0.5f, 0.5f, 0.4f), "", "carbonfiber.png");
//    Specular* refract = new Specular(texsys, 2.3f, float3(0.9f, 0.9f, 0.9f), "", "");
//    Phong* phong = new Phong(texsys, 2.5f, float3(0.f, 0.f, 0.f), float3(0.5f, 0.5f, 0.5f));
//    world->materials_.push_back(std::unique_ptr<Material>(matte0));
//    world->materials_.push_back(std::unique_ptr<Material>(matte1));
//    world->materials_.push_back(std::unique_ptr<Material>(phong));
//    world->materials_.push_back(std::unique_ptr<Material>(refract));
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//
//std::unique_ptr<World> BuildWorldFresnelTest(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    Bvh* bvh = new Sbvh(10.f, 8);
//    // Create camera
//    Camera* camera = new PerscpectiveCamera(float3(-4.f, 8.3f, -30.5f), float3(0.f,0.0,-3.5f), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    // Create lights
//    //PointLight* light1 = new PointLight(float3(0, 5, 5), 30.f * float3(3.f, 3.f, 3.f));
//    EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.6f);
//    //PointLight* light2 = new PointLight(float3(-5, 5, -2), 0.6 * float3(3.f, 2.9f, 2.4f));
//
//     // Add ground plane
//    float3 vertices[4] = {
//        float3(-5, 0, -5),
//        float3(-5, 0, 5),
//        float3(5, 0, 5),
//        float3(5, 0, -5)
//    };
//
//    float3 normals[4] = {
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0)
//    };
//
//    float2 uvs[4] = {
//        float2(0, 0),
//        float2(0, 4),
//        float2(4, 4),
//        float2(4, 0)
//    };
//
//    int indices[6] = {
//        0, 3, 1,
//        3, 1, 2
//    };
//
//    int materials[2] = {0,0};
//
//    matrix worldmat = translation(float3(0, -1.f, 0)) * scale(float3(10, 1, 10));
//
//    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
//                          &normals[0].x, 4, sizeof(float3),
//                          &uvs[0].x, 4, sizeof(float2),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          materials, sizeof(int),
//                          2, worldmat, inverse(worldmat));
//
//    std::vector<Primitive*> prims;
//    prims.push_back(mesh);
//
//    for (int i=0; i<5; ++i)
//        for (int j=0; j<5; ++j)
//        {
//                worldmat = translation(float3(-8.f+i*4, 0, -16.f+j*4));
//                prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1 + i + (j % 2) * 5));
//        }
//
//    bvh->Build(prims);
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    // Set background
//    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
//
//    // Build materials
//
//    Matte* matte0 = new Matte(texsys, float3(0.2f, 0.6f, 0.2f), "scratched.png", "");
//    world->materials_.push_back(std::unique_ptr<Material>(matte0));
//
//    for (int i=0; i<5; ++i)
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(new Glossy(texsys, float3(0.9f, 0.9f, 0.9f), 1.05f + 0.5f * i, 140.f, "", "")));
//    }
//
//    for (int i=0; i<5; ++i)
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(new Phong(texsys, 1.05f + 0.5f * i, float3(0.5f, 0.0f, 0.0f), float3(0.9f, 0.9f, 0.9f), "", "")));
//    }
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//
//std::unique_ptr<World> BuildWorldAntialias(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    Bvh* bvh = new Sbvh(10.f, 8);
//    // Create camera
//    Camera* camera = new PerscpectiveCamera(float3(0, 1.1f, -10.5f), float3(0, 1.1f,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    // Create lights
//    //PointLight* light1 = new PointLight(float3(0, 5, 5), 30.f * float3(3.f, 3.f, 3.f));
//    //PointLight* light2 = new PointLight(float3(-5, 5, -2), 0.6 * float3(3.f, 2.9f, 2.4f));
//        // Create lights
//    DirectionalLight* light1 = new DirectionalLight(float3(-1, -1, -1), 30.f * float3(3.f, 3.f, 3.f));
//
//     // Add ground plane
//    float3 vertices[4] = {
//        float3(-50, 0, -50),
//        float3(-50, 0, 50),
//        float3(50, 0, 50),
//        float3(50, 0, -50)
//    };
//
//    float3 normals[4] = {
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0)
//    };
//
//    float2 uvs[4] = {
//        float2(0, 0),
//        float2(0, 10),
//        float2(10, 10),
//        float2(10, 0)
//    };
//
//    int indices[6] = {
//        0, 3, 1,
//        3, 1, 2
//    };
//
//    int materials[2] = {0,0};
//
//    matrix worldmat = translation(float3(0, -1.f, 0)) * scale(float3(5, 1, 5));
//
//    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
//                          &normals[0].x, 4, sizeof(float3),
//                          &uvs[0].x, 4, sizeof(float2),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          materials, sizeof(int),
//                          2, worldmat, inverse(worldmat));
//
//    std::vector<Primitive*> prims;
//    prims.push_back(mesh);
//
//    bvh->Build(prims);
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    // Set background
//    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
//
//    // Build materials
//
//    Matte* matte0 = new Matte(texsys, float3(0.7f, 0.6f, 0.6f), "", "checker.png");
//    world->materials_.push_back(std::unique_ptr<Material>(matte0));
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//
//std::unique_ptr<World> BuildWorldCube(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    Bvh* bvh = new Sbvh(0.01f, 1);
//    // Create camera
//    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -8.5), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    Camera* camera = new PerscpectiveCamera(float3(0, 10, -25), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    // Create lights
//    //PointLight* light1 = new PointLight(float3(0, 5, 5), 30.f * float3(3.f, 3.f, 3.f));
//    EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.6f);
//    //PointLight* light2 = new PointLight(float3(-5, 5, -2), 0.6 * float3(3.f, 2.9f, 2.4f));
//
//     // Add ground plane
//    float3 vertices[4] = {
//        float3(-1, 0, -1),
//        float3(-1, 0, 1),
//        float3(1, 0, 1),
//        float3(1, 0, -1)
//    };
//
//    float3 normals[4] = {
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0)
//    };
//
//    float2 uvs[4] = {
//        float2(0, 0),
//        float2(0, 1),
//        float2(1, 1),
//        float2(1, 0)
//    };
//
//    int indices[6] = {
//        0, 3, 1,
//        3, 1, 2
//    };
//
//    int materials[2] = {0,0};
//
//    matrix worldmat = translation(float3(0, -1.f, 0)) * scale(float3(5, 1, 5));
//
//    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
//                          &normals[0].x, 4, sizeof(float3),
//                          &uvs[0].x, 4, sizeof(float2),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          materials, sizeof(int),
//                          2, worldmat, inverse(worldmat));
//
//    std::vector<Primitive*> prims;
//    prims.push_back(mesh);
//
//    bvh->Build(prims);
//
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    // Set background
//    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
//
//    // Build materials
//
//    Matte* matte0 = new Matte(texsys, float3(0.7f, 0.6f, 0.6f));
//
//    world->materials_.push_back(std::unique_ptr<Material>(matte0));
//
//
//    // Return world
//    return std::unique_ptr<World>(world);
//}



std::unique_ptr<World> BuildWorldAreaLightTest(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    Bvh* bvh = new Hlbvh();//new Sbvh(10.f, 8);
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(0, 3, -10.5), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);

    
    // Add ground plane
    float3 vertices[4] = {
        float3(-1, 0, -1),
        float3(-1, 0, 1),
        float3(1, 0, 1),
        float3(1, 0, -1)
    };
    
    float3 normals[4] = {
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 1, 0)
    };
    
    float3 negnormals[4] = {
        float3(0, -1, 0),
        float3(0, -1, 0),
        float3(0, -1, 0),
        float3(0, -1, 0)
    };
    
    float2 uvs[4] = {
        float2(0, 0),
        float2(0, 10),
        float2(10, 10),
        float2(10, 0)
    };
    
    int indices[6] = {
        0, 3, 1,
        3, 1, 2
    };
    
    int materials[2] = {0,0};
    
    matrix worldmat = translation(float3(0, -1.f, 0)) * scale(float3(50, 1, 50));
    
    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
                          &normals[0].x, 4, sizeof(float3),
                          &uvs[0].x, 4, sizeof(float2),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          materials, sizeof(int),
                          2, worldmat, inverse(worldmat));
    
    worldmat = translation(float3(0, 4.f, 0));
    int ematerials[2] = {2,2};
    Mesh* lightmesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
                          &negnormals[0].x, 4, sizeof(float3),
                          &uvs[0].x, 4, sizeof(float2),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          ematerials, sizeof(int),
                          2, worldmat, inverse(worldmat));

    std::vector<Primitive*> prims;
    prims.push_back(mesh);
    prims.push_back(lightmesh);

    worldmat = translation(float3(-2, 0, 0)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));

    worldmat = translation(float3(2, 0, 0));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));

    worldmat = translation(float3(-2, 0, -2.5)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));

    worldmat = translation(float3(2, 0, -2.5));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 3));

    worldmat = translation(float3(-2, 0, 2.5)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 3));

    worldmat = translation(float3(2, 0, 2.5));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 4));

    bvh->Build(prims);

    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Set background
    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);

    // Build materials
    SimpleMaterial* sm = new SimpleMaterial(new Lambert(texsys, float3(0.7f, 0.7f, 0.7f), "", ""));
    SimpleMaterial* sm1 = new SimpleMaterial(new Lambert(texsys, float3(0.7f, 0.2f, 0.2f), "", ""));
    SimpleMaterial* sm2 = new SimpleMaterial(new Microfacet(texsys, 2.5f, float3(0.1f, 0.8f, 0.2f), "", "", new FresnelDielectric(), new BlinnDistribution(100.f)));
    SimpleMaterial* sm3 = new SimpleMaterial(new PerfectReflect(texsys, 2.5f, float3(0.8f, 0.8f, 0.8f), "", ""));
    Emissive* emissive = new Emissive(float3(20.f, 18.f, 14.f));
    world->materials_.push_back(std::unique_ptr<Material>(sm));
    world->materials_.push_back(std::unique_ptr<Material>(sm1));
    world->materials_.push_back(std::unique_ptr<Material>(emissive));
    world->materials_.push_back(std::unique_ptr<Material>(sm2));
    world->materials_.push_back(std::unique_ptr<Material>(sm3));
    
    std::vector<Primitive*> meshprims;
    lightmesh->Refine(meshprims);

    for (int i=0; i<meshprims.size();++i)
    {
        world->lights_.push_back(std::unique_ptr<AreaLight>
                                 (
                                  new AreaLight(*meshprims[i], *emissive)
                                 )
                                 );
    }

    // Return world
    return std::unique_ptr<World>(world);
}


std::unique_ptr<World> BuildWorldIblTest(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    Bvh* bvh = new Sbvh(10.f, 8);
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(0, 10.f, -10.5f), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    
    EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.6f);
    
    // Add ground plane
    float3 vertices[4] = {
        float3(-1, 0, -1),
        float3(-1, 0, 1),
        float3(1, 0, 1),
        float3(1, 0, -1)
    };
    
    float3 normals[4] = {
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 1, 0)
    };
    
    float2 uvs[4] = {
        float2(0, 0),
        float2(0, 10),
        float2(10, 10),
        float2(10, 0)
    };
    
    int indices[6] = {
        0, 3, 1,
        3, 1, 2
    };
    
    int materials[2] = {0,0};
    
    matrix worldmat = translation(float3(0, -1.f, 0)) * scale(float3(50, 1, 50));
    
    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
                          &normals[0].x, 4, sizeof(float3),
                          &uvs[0].x, 4, sizeof(float2),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          materials, sizeof(int),
                          2, worldmat, inverse(worldmat));

    
    std::vector<Primitive*> prims;
    prims.push_back(mesh);
    
    worldmat = translation(float3(-2, 0, 0)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));
    
    worldmat = translation(float3(2, 0, 0));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));
    
    worldmat = translation(float3(-2, 0, -2.5)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 5));
    
    worldmat = translation(float3(2, 0, -2.5));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 3));
    
    worldmat = translation(float3(-2, 0, 2.5)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 3));
    
    worldmat = translation(float3(2, 0, 2.5));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 4));
    
    bvh->Build(prims);
    
    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Set background
    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
    // Add light
    world->lights_.push_back(std::unique_ptr<Light>(light1));
    
    // Build materials
    SimpleMaterial* sm = new SimpleMaterial(new Lambert(texsys, float3(0.7f, 0.7f, 0.7f), "", ""));
    SimpleMaterial* sm1 = new SimpleMaterial(new Lambert(texsys, float3(0.7f, 0.2f, 0.2f), "", ""));
    SimpleMaterial* sm2 = new SimpleMaterial(new Microfacet(texsys, 2.5f, float3(0.1f, 0.8f, 0.2f), "", "", new FresnelDielectric(), new BlinnDistribution(100.f)));
    SimpleMaterial* sm3 = new SimpleMaterial(new PerfectReflect(texsys, 2.5f, float3(0.8f, 0.8f, 0.8f), "", ""));
    SimpleMaterial* sm4 = new SimpleMaterial(new PerfectRefract(texsys, 2.5f, float3(0.8f, 0.8f, 0.8f), "", ""));
    Emissive* emissive = new Emissive(float3(20.f, 18.f, 14.f));
    world->materials_.push_back(std::unique_ptr<Material>(sm));
    world->materials_.push_back(std::unique_ptr<Material>(sm1));
    world->materials_.push_back(std::unique_ptr<Material>(emissive));
    world->materials_.push_back(std::unique_ptr<Material>(sm2));
    world->materials_.push_back(std::unique_ptr<Material>(sm3));
    world->materials_.push_back(std::unique_ptr<Material>(sm4));
    
    
    // Return world
    return std::unique_ptr<World>(world);
}

//std::unique_ptr<World> BuildWorldGlossy(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    //SimpleSet* set = new SimpleSet();
//    Bvh* bvh = new Sbvh(10.f, 8, false);
//    //Bvh* bvh = new Bvh();
//    // Create camera
//    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    Camera* camera = new PerscpectiveCamera(float3(1, 0, -1.1f), float3(0, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
//    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));
//    
//    // Create lights
//    PointLight* light1 = new PointLight(float3(1.f, 1.f, -1.f), float3(0.97f, 0.97f, 0.97f));
//    
//    rand_init();
//    
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
//    AssimpAssetImporter assimp(texsys, "../../../Resources/dragon/dragon1.obj");
//    
//    assimp.onmaterial_ = [&world](Material* mat)->int
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(mat));
//        return (int)(world->materials_.size() - 1);
//    };
//    
//    std::vector<Primitive*> primitives;
//    assimp.onprimitive_ = [&primitives](Primitive* prim)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        primitives.push_back(prim);
//    };
//    
//    // Start assets import
//    assimp.Import();
//    
//    // Add ground plane
//    float3 vertices[4] = {
//        float3(-1, 0, -1),
//        float3(-1, 0, 1),
//        float3(1, 0, 1),
//        float3(1, 0, -1)
//    };
//    
//    float3 normals[4] = {
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0)
//    };
//    
//    float2 uvs[4] = {
//        float2(0, 0),
//        float2(0, 1),
//        float2(1, 1),
//        float2(1, 0)
//    };
//    
//    int indices[6] = {
//        0, 3, 1,
//        3, 1, 2
//    };
//    
//    // Clear default material
//    world->materials_.clear();
//    
//    //world->materials_.push_back(std::unique_ptr<Material>(new Refract(texsys, 2.3f, float3(0.9f, 0.3f, 0.0f))));
//    
//    world->materials_.push_back(std::unique_ptr<Material>(new Glossy(texsys, float3(0.9f, 0.9f, 0.9f), 2.5f, 5.f, "", "")));
//    world->materials_.push_back(std::unique_ptr<Material>(new Matte(texsys, float3(0.8f, 0.8f, 0.8f))));
//    
//    int materials[2] = {1, 1};
//    
//    matrix worldmat = translation(float3(0, -0.28f, 0)) * scale(float3(5, 1, 5));
//    
//    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
//                          &normals[0].x, 4, sizeof(float3),
//                          &uvs[0].x, 4, sizeof(float2),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          materials, sizeof(int),
//                          2, worldmat, inverse(worldmat));
//    
//    primitives.push_back(mesh);
//    
//    // Build acceleration structure
//    auto starttime = std::chrono::high_resolution_clock::now();
//    bvh->Build(primitives);
//    auto endtime = std::chrono::high_resolution_clock::now();
//    auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);
//    
//    std::cout << "Acceleration structure constructed in " << exectime.count() << " ms\n";
//    
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    //world->accelerator_ = std::unique_ptr<Primitive>(set);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    //world->lights_.push_back(std::unique_ptr<Light>(light2));
//    // Set background
//    world->bgcolor_ = float3(0.4f, 0.4f, 0.4f);
//    
//    // Return world
//    return std::unique_ptr<World>(world);
//}
//
//std::unique_ptr<World> BuildWorldMicrofacet(TextureSystem const& texsys)
//{
//    // Create world
//    World* world = new World();
//    // Create accelerator
//    //SimpleSet* set = new SimpleSet();
//    Bvh* bvh = new Sbvh(10.f, 8, false);
//    //Bvh* bvh = new Bvh();
//    // Create camera
//    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
//    Camera* camera = new PerscpectiveCamera(float3(1, 0, -1.1f), float3(0, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
//    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));
//    
//    // Create lights
//    PointLight* light1 = new PointLight(float3(1.f, 1.f, -1.f), float3(0.97f, 0.85f, 0.55f));
//    
//    rand_init();
//    
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
//    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
//    AssimpAssetImporter assimp(texsys, "../../../Resources/dragon/dragon1.obj");
//    
//    assimp.onmaterial_ = [&world](Material* mat)->int
//    {
//        world->materials_.push_back(std::unique_ptr<Material>(mat));
//        return (int)(world->materials_.size() - 1);
//    };
//    
//    std::vector<Primitive*> primitives;
//    assimp.onprimitive_ = [&primitives](Primitive* prim)
//    //assimp.onprimitive_ = [&set](Primitive* prim)
//    {
//        //set->Emplace(prim);
//        primitives.push_back(prim);
//    };
//    
//    // Start assets import
//    assimp.Import();
//    
//    // Add ground plane
//    float3 vertices[4] = {
//        float3(-1, 0, -1),
//        float3(-1, 0, 1),
//        float3(1, 0, 1),
//        float3(1, 0, -1)
//    };
//    
//    float3 normals[4] = {
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0),
//        float3(0, 1, 0)
//    };
//    
//    float2 uvs[4] = {
//        float2(0, 0),
//        float2(0, 1),
//        float2(1, 1),
//        float2(1, 0)
//    };
//    
//    int indices[6] = {
//        0, 3, 1,
//        3, 1, 2
//    };
//    
//    // Clear default material
//    world->materials_.clear();
//    
//    //world->materials_.push_back(std::unique_ptr<Material>(new Refract(texsys, 2.3f, float3(0.9f, 0.3f, 0.0f))));
//    
//    world->materials_.push_back(std::unique_ptr<Material>(new Glossy(texsys, float3(0.7f, 0.2f, 0.2f), 1.5f, 10.f, "", "")));
//    world->materials_.push_back(std::unique_ptr<Material>(new Matte(texsys, float3(0.8f, 0.8f, 0.8f))));
//    
//    int materials[2] = {1, 1};
//    
//    matrix worldmat = translation(float3(0, -0.28f, 0)) * scale(float3(5, 1, 5));
//    
//    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
//                          &normals[0].x, 4, sizeof(float3),
//                          &uvs[0].x, 4, sizeof(float2),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          indices, sizeof(int),
//                          materials, sizeof(int),
//                          2, worldmat, inverse(worldmat));
//    
//    primitives.push_back(mesh);
//    
//    // Build acceleration structure
//    auto starttime = std::chrono::high_resolution_clock::now();
//    bvh->Build(primitives);
//    auto endtime = std::chrono::high_resolution_clock::now();
//    auto exectime = std::chrono::duration_cast<std::chrono::milliseconds>(endtime - starttime);
//    
//    std::cout << "Acceleration structure constructed in " << exectime.count() << " ms\n";
//    
//    // Attach accelerator to world
//    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
//    //world->accelerator_ = std::unique_ptr<Primitive>(set);
//    // Attach camera
//    world->camera_ = std::unique_ptr<Camera>(camera);
//    // Attach point lights
//    world->lights_.push_back(std::unique_ptr<Light>(light1));
//    //world->lights_.push_back(std::unique_ptr<Light>(light2));
//    // Set background
//    world->bgcolor_ = float3(0.4f, 0.4f, 0.4f);
//    
//    // Return world
//    return std::unique_ptr<World>(world);
//}


int main()
{
    try
    {
        // Init RNG
        rand_init();

        // File name to render
        std::string filename = "result.png";
        int2 imgres = int2(512, 512);
        // Create texture system
        OiioTextureSystem texsys("../../../Resources/Textures");

        // Build world
        std::cout << "Constructing world...\n";
        std::unique_ptr<World> world = BuildWorldAreaLightTest(texsys);

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
            new GiTracer(10, 1.f), // Tracer
            new StratifiedSampler(8, new McRng()), // Image sampler
            //new RegularSampler(2),
            new StratifiedSampler(1, new McRng()), // Light sampler
            new RandomSampler(1, new McRng()), // Brdf sampler
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
        std::cout << e.what() << "\n";
    }

    return 0;
}
