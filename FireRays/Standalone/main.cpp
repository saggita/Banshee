/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/
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
#include "accelerator/bvh.h"
#include "imageio/oiioimageio.h"
#include "camera/perspective_camera.h"
#include "camera/environment_camera.h"
#include "renderer/imagerenderer.h"
#include "renderer/mt_imagerenderer.h"
#include "renderer/adaptive_renderer.h"
#include "imageplane/fileimageplane.h"
#include "tracer/ditracer.h"
#include "tracer/gitracer.h"
#include "tracer/aotracer.h"
#include "tracer/shtracer.h"
#include "light/pointlight.h"
#include "light/directional_light.h"
#include "light/environment_light.h"
#include "light/environment_light_is.h"
#include "light/arealight.h"
#include "sampler/random_sampler.h"
#include "sampler/regular_sampler.h"
#include "sampler/stratified_sampler.h"
#include "rng/mcrng.h"
#include "material/simplematerial.h"
#include "material/emissive.h"
#include "material/glass.h"
#include "material/mixedmaterial.h"
#include "bsdf/lambert.h"
#include "bsdf/microfacet.h"
#include "bsdf/perfect_reflect.h"
#include "bsdf/perfect_refract.h"
#include "bsdf/normal_mapping.h"
#include "texture/oiio_texturesystem.h"
#include "import/assimp_assetimporter.h"
#include "util/progressreporter.h"
#include "math/sh.h"
#include "math/shproject.h"



std::unique_ptr<World> BuildWorld(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    Bvh* bvh = new Bvh();
    //Grid* grid = new Grid();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(0.0f, 1.0f, 3.5f), float3(0.0f, 1.0f, 0), float3(0, 1.f, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 0, 0), float3(1, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(0,-1,0), float3(0, 0, 1), float2(0.01f, 10000.f));

    rand_init();

    AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/crytek-sponza/sponza.obj");

    assimp.onmaterial_ = [&world, &texsys](Material* mat)->int
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

    assimp.onlight_ = [&world](Light* light)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        world->lights_.push_back(std::unique_ptr<Light>(light));
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
    // Set background
    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);

    // Return world
    return std::unique_ptr<World>(world);
}

std::unique_ptr<World> BuildWorldBlender(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    Bvh* bvh = new Bvh();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(-20.5, 15.0f, 10.f), float3(0, 5.0f, 0), float3(0, 1.f, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 0, 0), float3(1, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(0,-1,0), float3(0, 0, 1), float2(0.01f, 10000.f));
    EnvironmentLightIs* light1 = new EnvironmentLightIs(texsys, "Apartment.hdr", 0.3f);

    rand_init();

    AssimpAssetImporter assimp(texsys, "../../../Resources/contest/blender.obj");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/crytek-sponza/sponza.obj");

    assimp.onmaterial_ = [&world, &texsys](Material* mat)->int
    {
        world->materials_.push_back(std::unique_ptr<Material>(new Glass(texsys, 1.5f, float3(0.75f, 0.55f, 0.6f))));

        return (int)(world->materials_.size() - 1);
    };

    std::vector<Primitive*> primitives;
    assimp.onprimitive_ = [&primitives](Primitive* prim)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        primitives.push_back(prim);
    };

    assimp.onlight_ = [&world](Light* light)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        world->lights_.push_back(std::unique_ptr<Light>(light));
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
    // Set background
    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
    // Attach light
    world->lights_.push_back(std::unique_ptr<Light>(light1));

    // Return world
    return std::unique_ptr<World>(world);
}
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
    Bvh* bvh = new Bvh();
    //Bvh* bvh = new Bvh();
    // Create camera
    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    Camera* camera = new PerscpectiveCamera(float3(1, 0, -1.1f), float3(0, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));

    // Create lights
    //PointLight* light1 = new PointLight(float3(1.f, 1.f, -1.f), float3(0.85f, 0.85f, 0.85f));
    //EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.8f);
    EnvironmentLightIs* light1 = new EnvironmentLightIs(texsys, "Apartment.hdr", 0.8f);

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

    world->materials_.push_back(std::unique_ptr<Material>(new Glass(texsys, 1.5f, float3(0.7f, 0.7f, 0.7f))));
    //world->materials_.push_back(std::unique_ptr<Material>(new SimpleMaterial(
                                                                             //new Microfacet(texsys, 2.f, float3(0.7f, 0.7f, 0.7f), "", "", new FresnelDielectric(), new BlinnDistribution(100.f)))));
    
    world->materials_.push_back(std::unique_ptr<Material>(new SimpleMaterial(
                                                                             new Microfacet(texsys, 5.f, float3(0.3f, 0.7f, 0.3f), "", "", new FresnelDielectric(), new BlinnDistribution(300.f)))));

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

std::unique_ptr<World> BuildWorldMitsuba(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    Bvh* bvh = new Bvh();
    //Bvh* bvh = new Bvh();
    // Create camera
    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    Camera* camera = new PerscpectiveCamera(float3(1, 2, 5.5f), float3(0, 0, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));

    // Create lights
    //PointLight* light1 = new PointLight(float3(1.f, 1.f, -1.f), float3(0.85f, 0.85f, 0.85f));
    //EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.8f);
    EnvironmentLightIs* light1 = new EnvironmentLightIs(texsys, "Apartment.hdr", 1.2f);

    rand_init();

    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
    AssimpAssetImporter assimp(texsys, "../../../Resources/mitsuba/mitsuba.obj");

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

    //world->materials_[2].reset(new Glass(texsys, 1.5f, float3(0.7f, 0.8f, 0.75f)));

    world->materials_[2].reset(new SimpleMaterial(new Microfacet(texsys, 3.f, float3(0.7f, 0.7f, 0.7f), "", "", new FresnelDielectric(), new BlinnDistribution(600.f))));

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

    //primitives.push_back(mesh);

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
    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);

    // Return world
    return std::unique_ptr<World>(world);
}

std::unique_ptr<World> BuildWorldClassroom(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    //SimpleSet* set = new SimpleSet();
    Bvh* bvh = new Bvh();
    //Bvh* bvh = new Bvh();
    // Create camera
    //Camera* camera = new PerscpectiveCamera(float3(0, 1, 4), float3(0, 1, 0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    Camera* camera = new PerscpectiveCamera(float3(-135.f, 110.f, 95.f), float3(-200.f, 110.f, 95.f), float3(0, 1, 0), float2(0.01f, 100000.f), PI / 3, 1.f);
    //Camera* camera = new EnvironmentCamera(float3(0, 0, 0), float3(1,0,0), float3(0, 1, 0), float2(0.01f, 10000.f));

    // Create lights
    //PointLight* light0 = new PointLight(float3(-300.f, 150.f, 95.f), float3(1000.85f, 1000.85f, 1000.85f));
    //EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.8f);
    EnvironmentLightIs* light1 = new EnvironmentLightIs(texsys, "Apartment.hdr", 5.8f);

    rand_init();

    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/orig.obj");
    //AssimpAssetImporter assimp(texsys, "../../../Resources/cornell-box/CornellBox-Glossy.obj");
    AssimpAssetImporter assimp(texsys, "../../../Resources/Contest/classroom.obj");

    assimp.onmaterial_ = [&world](Material* mat)->int
    {
        world->materials_.push_back(std::unique_ptr<Material>(mat));
        return (int)(world->materials_.size() - 1);
    };

    std::vector<Primitive*> primitives;
    assimp.onprimitive_ = [&primitives](Primitive* prim)
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
    //world->materials_.clear();

    world->materials_.push_back(std::unique_ptr<Material>(new Glass(texsys, 1.5f, float3(0.7f, 0.7f, 0.7f))));

    world->materials_.push_back(std::unique_ptr<Material>(new SimpleMaterial(
                                                                             new Microfacet(texsys, 5.f, float3(0.3f, 0.7f, 0.3f), "", "", new FresnelDielectric(), new BlinnDistribution(300.f)))));

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

    //primitives.push_back(mesh);

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
    //world->lights_.push_back(std::unique_ptr<Light>(light0));
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
    Bvh* bvh = new Bvh();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(3.5f, 5.f, -10.5), float3(0.f, 0.f,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
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
    
    worldmat = translation(float3(3.f, 4.f, 0));
    
    Mesh* lightmesh1 = new Mesh(&vertices[0].x, 4, sizeof(float3),
                               &negnormals[0].x, 4, sizeof(float3),
                               &uvs[0].x, 4, sizeof(float2),
                               indices, sizeof(int),
                               indices, sizeof(int),
                               indices, sizeof(int),
                               ematerials, sizeof(int),
                               2, worldmat, inverse(worldmat));
    
    worldmat = translation(float3(-3.f, 4.f, 0));
    
    Mesh* lightmesh2 = new Mesh(&vertices[0].x, 4, sizeof(float3),
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
    prims.push_back(lightmesh1);
    prims.push_back(lightmesh2);

    worldmat = translation(float3(-2, 0, 0)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));

    worldmat = translation(float3(2, 0, 0));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 4));

    worldmat = translation(float3(-2, 0, -2.5)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 4));

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
    SimpleMaterial* sm = new SimpleMaterial(new NormalMapping(new Lambert(texsys, float3(0.7f, 0.7f, 0.7f), "kamen.png"), "kamen-bump.png"));
    SimpleMaterial* sm1 = new SimpleMaterial(new Lambert(texsys, float3(0.7f, 0.2f, 0.2f), "", ""));
    SimpleMaterial* sm2 = new SimpleMaterial(new Microfacet(texsys, 5.5f, float3(0.7f, 0.7f, 0.7f), "", "", new FresnelDielectric(), new BlinnDistribution(500.f)));
    Glass* sm3 = new Glass(texsys, 2.5f, float3(0.4f, 0.8f, 0.4f), "");
    Emissive* emissive = new Emissive(float3(13.f, 11.f, 8.f));
    world->materials_.push_back(std::unique_ptr<Material>(sm));
    world->materials_.push_back(std::unique_ptr<Material>(sm1));
    world->materials_.push_back(std::unique_ptr<Material>(emissive));
    world->materials_.push_back(std::unique_ptr<Material>(sm2));
    world->materials_.push_back(std::unique_ptr<Material>(sm3));
    
    std::vector<Primitive*> meshprims;
    lightmesh->Refine(meshprims);
    lightmesh1->Refine(meshprims);
    lightmesh2->Refine(meshprims);
    

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
    Bvh* bvh = new Bvh();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(4.f, 5.0f, -10.5f), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 800.f/600.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);

    //EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.6f);
    EnvironmentLightIs* light1 = new EnvironmentLightIs(texsys, "Apartment.hdr", 0.8f);

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
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 5));
    
    worldmat = translation(float3(2, 0, 0));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 1));
    
    worldmat = translation(float3(-2, 0, -2.5)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 5));
    
    worldmat = translation(float3(2, 0, -2.5));
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 6));
    
    worldmat = translation(float3(-2, 0, 2.5)) * rotation_x(PI/2);
    prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 6));
    
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
    SimpleMaterial* sm = new SimpleMaterial(new Microfacet(texsys, 2.5f, float3(0.3f, 0.4f, 0.3f), "", "", new FresnelDielectric(), new BlinnDistribution(200.f)));
    SimpleMaterial* sm1 = new SimpleMaterial(new NormalMapping(new Lambert(texsys, float3(0.7f, 0.2f, 0.2f), "", ""), "carbonfiber.png"));
    SimpleMaterial* sm2 = new SimpleMaterial(new Microfacet(texsys, 2.5f, float3(0.1f, 0.8f, 0.2f), "", "", new FresnelDielectric(), new BlinnDistribution(100.f)));
    SimpleMaterial* sm3 = new SimpleMaterial(new NormalMapping(new PerfectReflect(texsys, 3.5f, float3(0.8f, 0.8f, 0.8f), "", "", new FresnelDielectric()), "carbonfiber.png"));
    Glass* sm4 = new Glass(texsys, 2.5f, float3(0.8f, 0.8f, 0.8f), "");
    Emissive* emissive = new Emissive(float3(20.f, 18.f, 14.f));
    world->materials_.push_back(std::unique_ptr<Material>(sm));
    world->materials_.push_back(std::unique_ptr<Material>(sm1));
    world->materials_.push_back(std::unique_ptr<Material>(emissive));
    world->materials_.push_back(std::unique_ptr<Material>(sm2));
    world->materials_.push_back(std::unique_ptr<Material>(sm3));
    world->materials_.push_back(std::unique_ptr<Material>(sm4));
    
    MixedMaterial* mm = new MixedMaterial(3.5f);
    mm->AddBsdf(new Microfacet(texsys, 5.5f, float3(0.8f, 0.6f, 0.4f), "", "", new FresnelDielectric(), new BlinnDistribution(400.f)));
    mm->AddBsdf(new PerfectRefract(texsys, 5.5f, float3(0.8f, 0.8f, 0.8f), "", ""));
    world->materials_.push_back(std::unique_ptr<Material>(mm));


    // Return world
    return std::unique_ptr<World>(world);
}

std::unique_ptr<World> BuildWorldShTest(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    Bvh* bvh = new Bvh();
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(1, 0, -1.1f), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    EnvironmentLight* light1 = new EnvironmentLight(texsys, "Apartment.hdr", 0.6f);
    
    std::vector<Primitive*> prims;
    
    //matrix worldmat = translation(float3(0,0,0));//rotation_x(PI/2);
    //prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 0));
    
    AssimpAssetImporter assimp(texsys, "../../../Resources/dragon/dragon1.obj");
    
    assimp.onmaterial_ = [&world](Material* mat)->int
    {
        world->materials_.push_back(std::unique_ptr<Material>(mat));
        return (int)(world->materials_.size() - 1);
    };
    
    assimp.onprimitive_ = [&prims](Primitive* prim)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        prims.push_back(prim);
    };
    
    // Start assets import
    assimp.Import();
    
    bvh->Build(prims);
    
    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Set background
    world->bgcolor_ = float3(0.0f, 0.05f, 0.0f);
    // Add light
    world->lights_.push_back(std::unique_ptr<Light>(light1));

    
    // Build materials
    SimpleMaterial* sm = new SimpleMaterial(new Lambert(texsys, float3(0.7f, 0.7f, 0.7f), "", ""));
    world->materials_.push_back(std::unique_ptr<Material>(sm));
    
    // Return world
    return std::unique_ptr<World>(world);
}


std::unique_ptr<World> BuildWorldIblTest1(TextureSystem const& texsys)
{
    // Create world
    World* world = new World();
    // Create accelerator
    Bvh* bvh = new Bvh(true);
    // Create camera
    Camera* camera = new PerscpectiveCamera(float3(-2, 0.5, -1.1f), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    //Camera* camera = new PerscpectiveCamera(float3(0, 3, -4.5), float3(-2,1,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
    EnvironmentLightIs* light1 = new EnvironmentLightIs(texsys, "Harbor_3_Free_Ref.hdr", 1.2f, 1.f);
    DirectionalLight* light2 = new DirectionalLight(float3(-0.5f, -1.f, 0.75f), float3(4,4,4));
    
    std::vector<Primitive*> prims;
    
    //matrix worldmat = translation(float3(0,0,0));//rotation_x(PI/2);
    //prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 0));
    
    AssimpAssetImporter assimp(texsys, "../../../Resources/dragon/dragon1.obj");
    
    assimp.onmaterial_ = [&world, &texsys](Material* mat)->int
    {
        world->materials_.push_back(std::unique_ptr<Material>(new SimpleMaterial(new Lambert(texsys, float3(0.6f, 0.6f, 0.6f)))));
        //world->materials_.push_back(std::unique_ptr<Material>(new Glass(texsys, 1.5f, float3(0.9f, 0.9f, 0.9f))));
        return (int)(world->materials_.size() - 1);
    };
    
    assimp.onprimitive_ = [&prims](Primitive* prim)
    //assimp.onprimitive_ = [&set](Primitive* prim)
    {
        //set->Emplace(prim);
        prims.push_back(prim);
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
    
    int materials[2] = {1, 1};
    
    matrix worldmat = translation(float3(0, -0.28f, 0)) * scale(float3(3, 1, 3));
    
    Mesh* mesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
                          &normals[0].x, 4, sizeof(float3),
                          &uvs[0].x, 4, sizeof(float2),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          indices, sizeof(int),
                          materials, sizeof(int),
                          2, worldmat, inverse(worldmat));
    
    prims.push_back(mesh);
    
    bvh->Build(prims);
    
    // Attach accelerator to world
    world->accelerator_ = std::unique_ptr<Primitive>(bvh);
    // Attach camera
    world->camera_ = std::unique_ptr<Camera>(camera);
    // Set background
    world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
    // Add light
    world->lights_.push_back(std::unique_ptr<Light>(light1));
    world->lights_.push_back(std::unique_ptr<Light>(light2));
    
    
    // Build materials
    //SimpleMaterial* sm = new SimpleMaterial(new PerfectRefract(texsys, 1.4f, float3(0.7f, 0.7f, 0.7f), "", ""));
    world->materials_.push_back(std::unique_ptr<Material>(new SimpleMaterial(new Lambert(texsys, float3(0.6f, 0.6f, 0.6f)))));
    
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


int main_1()
{
    try
    {
        // Init RNG
        rand_init();

        // File name to render
        std::string filename = "result.png";
        int2 imgres = int2(1024, 1024);
        // Create texture system
        OiioTextureSystem texsys("../../../Resources/Textures");

        // Build world
        std::cout << "Constructing world...\n";
        std::unique_ptr<World> world = BuildWorld(texsys);

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
                    std::cout << percents << "%... " << std::flush;
                    prevprogress_ = percents;
                }
            }
        private:
            int prevprogress_;
        };
        
        // Create renderer w/ direct illumination trace
        std::cout << "Kicking off rendering engine...\n";
        MtImageRenderer renderer(plane, // Image plane
            new GiTracer(3), // Tracer
            new StratifiedSampler(8, new McRng()), // Image sampler
            new StratifiedSampler(1, new McRng()), // Light sampler
            new StratifiedSampler(1, new McRng()), // Brdf sampler
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


#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLUT/GLUT.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#include <thread>
#include <mutex>

#include "shader_manager.h"

int g_window_width = 600;
int g_window_height = 600;
std::unique_ptr<ShaderManager>	g_shader_manager;

std::vector<char> g_data;
std::mutex g_data_mutex;

GLuint g_vertex_buffer;
GLuint g_index_buffer;
GLuint g_texture;

void Display()
{
    try
    {
        {
            glDisable(GL_DEPTH_TEST);
            glViewport(0, 0, g_window_width, g_window_height);

            glClear(GL_COLOR_BUFFER_BIT);

            glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);

            GLuint program = g_shader_manager->GetProgram("../../../Standalone/simple");
            glUseProgram(program);

            GLuint texloc = glGetUniformLocation(program, "g_Texture");
            assert(texloc >= 0);

            glUniform1i(texloc, 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_texture);

            GLuint position_attr = glGetAttribLocation(program, "inPosition");
            GLuint texcoord_attr = glGetAttribLocation(program, "inTexcoord");

            glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(float)*5, 0);
            glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(float)*5, (void*)(sizeof(float) * 3));
            
            glEnableVertexAttribArray(position_attr);
            glEnableVertexAttribArray(texcoord_attr);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
            
            glDisableVertexAttribArray(texcoord_attr);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glUseProgram(0);
        }

        glutSwapBuffers();
    }
    catch (std::runtime_error& e)
    {
        std::cout << e.what();
        exit(-1);
    }
}

void ResizeBuffers()
{
}

void InitGraphics()
{
    g_shader_manager.reset(new ShaderManager());

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glCullFace(GL_NONE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glGenBuffers(1, &g_vertex_buffer);
    glGenBuffers(1, &g_index_buffer);

    // create Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);

    float quad_vdata[] =
    {
        -1, -1, 0.5, 0, 0,
        1, -1, 0.5, 1, 0,
        1,  1, 0.5, 1, 1,
        -1,  1, 0.5, 0, 1
    };

    GLshort quad_idata[] =
    {
        0, 1, 3,
        3, 1, 2
    };

    // fill data
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vdata), quad_vdata, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idata), quad_idata, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_window_width, g_window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Update()
{
    static auto prevtime = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::duration<double> >(time - prevtime);
    prevtime = time;

    bool update = false;
    

    glActiveTexture(GL_TEXTURE0);
    
    glBindTexture(GL_TEXTURE_2D, g_texture);

    {
        std::unique_lock<std::mutex> lock(g_data_mutex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_window_width, g_window_height, GL_RGB, GL_UNSIGNED_BYTE, &g_data[0]);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glutPostRedisplay();
}

void Reshape(GLint w, GLint h)
{
    /*g_window_width = w;
    g_window_height = h;
    
    glDeleteTextures(1, &g_texture);
    
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_window_width, g_window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    ResizeBuffers();
    
    glutPostRedisplay();*/

    // Disable window resize for now
    glutReshapeWindow(g_window_width, g_window_height);
}


class BufferImagePlane : public ImagePlane
{
public:
    BufferImagePlane(int2 res)
        :  res_(res)
        , imgbuf_(res.x * res.y)
    {
        std::fill(imgbuf_.begin(), imgbuf_.end(), float3(0.f, 0.f, 0.f, 0.f));
    }

    // This method is called by the renderer prior to adding samples
    void Prepare()
    {
    }
    

    // This method is called by the renderer after adding all the samples
    void Finalize()
    {
    }

    // Add color contribution to the image plane
    void AddSample(int2 const& pos, float w, float3 value)
    {
        int2   imgpos;

        // Make sure we are in the image space as (<0.5f,<0.5f) might map outside of the image
        imgpos.x = (int)clamp((float)pos.x, 0.f, (float)res_.x-1);
        imgpos.y = (int)clamp((float)pos.y, 0.f, (float)res_.y-1);

        imgbuf_[res_.x * (res_.y - 1 - imgpos.y) + imgpos.x] += w * value;
        imgbuf_[res_.x * (res_.y - 1 - imgpos.y) + imgpos.x].w += 1.f;
    }

    // This is used by the renderer to decide on the number of samples needed
    int2 resolution() const { return res_; }

    // Image resolution
    int2 res_;
    // Intermediate image buffer
    std::vector<float3> imgbuf_;
};

void Render()
{
    OiioTextureSystem texsys("../../../Resources/Textures");
    std::unique_ptr<World> world = BuildWorldIblTest1(texsys);

    // Create OpenImageIO based IO api
    OiioImageIo io;
    // Create image plane writing to file

    // Create renderer w/ direct illumination trace
    std::cout << "Kicking off rendering engine...\n";
    
    BufferImagePlane plane(int2(g_window_width, g_window_height));

    MtImageRenderer renderer(plane, // Image plane
        new GiTracer(10), // Tracer
        new RandomSampler(2, new McRng()), // Image sampler
        new StratifiedSampler(2, new McRng()), // Light sampler
        new StratifiedSampler(2, new McRng()), // Brdf sampler
        nullptr // Progress reporter
        );

    int numpasses = 0;
    while (1)
    {
        ++numpasses;
        renderer.Render(*world);

        {
            std::unique_lock<std::mutex> lock(g_data_mutex);
            for (int i = 0; i < g_window_width * g_window_height; ++i)
            {
                g_data[3 * i] = (char)(255 * clamp(powf(plane.imgbuf_[i].x / plane.imgbuf_[i].w, 1.f / 2.2f), 0.f, 1.f));
                g_data[3 * i + 1] = (char)(255 * clamp(powf(plane.imgbuf_[i].y / plane.imgbuf_[i].w, 1.f / 2.2f), 0.f, 1.f));
                g_data[3 * i + 2] = (char)(255 * clamp(powf(plane.imgbuf_[i].z / plane.imgbuf_[i].w, 1.f / 2.2f), 0.f, 1.f));
            }
        }
        
        std::cout << "Pass " << numpasses << " finished\n";
    }
}


int main(int argc, char** argv)
{
     // GLUT Window Initialization:
    glutInit (&argc, (char**)argv);
    glutInitWindowSize (g_window_width, g_window_height);
    glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow ("App");
    
#ifndef __APPLE__
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "GLEW initialization failed\n";
        return -1;
    }
#endif
    
    try
    {
        g_data.resize(g_window_height * g_window_width * 3);

        InitGraphics();
        
        std::thread t(Render);

        t.detach();

        // Register callbacks:
        glutDisplayFunc (Display);
        glutReshapeFunc (Reshape);
        glutIdleFunc (Update);
        
        glutMainLoop ();
    }
    catch(std::runtime_error& err)
    {
        std::cout << err.what();
        return -1;
    }

}
