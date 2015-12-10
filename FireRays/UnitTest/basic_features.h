#ifndef BANSHEE_TEST
#define BANSHEE_TEST

/// Simplest functionality tests

#include <gtest/gtest.h>

#include "imgcompare.h"

#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "math/mathutils.h"
#include "world/world.h"
#include "primitive/mesh.h"
#include "accelerator/bvh.h"
#include "imageio/oiioimageio.h"
#include "camera/perspective_camera.h"
#include "camera/environment_camera.h"
#include "renderer/mt_imagerenderer.h"
#include "imageplane/fileimageplane.h"
#include "tracer/ditracer.h"
#include "light/pointlight.h"
#include "light/directional_light.h"
#include "light/arealight.h"
#include "light/environment_light.h"
#include "sampler/regular_sampler.h"
#include "sampler/stratified_sampler.h"
#include "material/simplematerial.h"
#include "material/emissive.h"
#include "bsdf/lambert.h"
#include "texture/oiio_texturesystem.h"
#include "import/assimp_assetimporter.h"
#include "util/progressreporter.h"
#include "math/sh.h"
#include "math/shproject.h"
#include "rng/mcrng.h"

extern std::string g_output_image_path;
extern std::string g_ref_image_path;
extern std::string g_texture_path;
extern int2 g_imgres;
extern bool g_compare;
extern int  g_num_spp;

class BasicFeatures : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        // Texture system
        texsys_.reset(new OiioTextureSystem(g_texture_path));
        
        // Build default world
        world_ = BuildWorld();
        
        // Comparator
        imgcmp_.reset(new ImgCompare(io_));
    }

    virtual void TearDown()
    {
        
    }
    
    virtual std::unique_ptr<World> BuildWorld() const
    {
        // Create world
        World* world = new World();
        // Create accelerator
        Bvh* bvh = new Bvh();
        // Create camera
        Camera* camera = new PerscpectiveCamera(float3(0.f, 2.f, -10.5f), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
        
        // Simple point light
        PointLight* light = new PointLight(float3(0,5,0), float3(30,30,30));
        
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
        
        worldmat = rotation_x(PI/2);
        prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 0));
        
        bvh->Build(prims);
        
        // Attach accelerator to world
        world->accelerator_ = std::unique_ptr<Primitive>(bvh);
        // Attach camera
        world->camera_ = std::unique_ptr<Camera>(camera);
        // Set background
        world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
        // Add light
        world->lights_.push_back(std::unique_ptr<Light>(light));
        
        // Build materials
        SimpleMaterial* sm = new SimpleMaterial(new Lambert(*texsys_, float3(0.7f, 0.7f, 0.7f), "", ""));
        // Add to scene
        world->materials_.push_back(std::unique_ptr<Material>(sm));
        
        // Return world
        return std::unique_ptr<World>(world);
    }
    
    
    virtual std::unique_ptr<World> BuildWorldAreaLight() const
    {
        // Create world
        World* world = new World();
        // Create accelerator
        Bvh* bvh = new Bvh();
        // Create camera
        Camera* camera = new PerscpectiveCamera(float3(0.f, 2.f, -10.5f), float3(0,0,0), float3(0, 1, 0), float2(0.01f, 10000.f), PI / 4, 1.f);
        
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
        
        
        worldmat = translation(float3(0, 3.f, 0)) * rotation_x((float)M_PI);
        
        int ematerials[2] = {1,1};
        Mesh* lightmesh = new Mesh(&vertices[0].x, 4, sizeof(float3),
                                   &normals[0].x, 4, sizeof(float3),
                                   &uvs[0].x, 4, sizeof(float2),
                                   indices, sizeof(int),
                                   indices, sizeof(int),
                                   indices, sizeof(int),
                                   ematerials, sizeof(int),
                                   2, worldmat, inverse(worldmat));
        
        
        std::vector<Primitive*> prims;
        prims.push_back(mesh);
        prims.push_back(lightmesh);
        
        worldmat = rotation_x(PI/2);
        prims.push_back(new Sphere(1.f, worldmat, inverse(worldmat), 0));
        
        bvh->Build(prims);
        
        // Attach accelerator to world
        world->accelerator_ = std::unique_ptr<Primitive>(bvh);
        // Attach camera
        world->camera_ = std::unique_ptr<Camera>(camera);
        // Set background
        world->bgcolor_ = float3(0.0f, 0.0f, 0.0f);
        
        // Build materials
        SimpleMaterial* sm = new SimpleMaterial(new Lambert(*texsys_, float3(0.7f, 0.7f, 0.7f), "", ""));
        Emissive* emissive = new Emissive(float3(20.f, 18.f, 14.f));
        // Add to scene
        world->materials_.push_back(std::unique_ptr<Material>(sm));
        world->materials_.push_back(std::unique_ptr<Material>(emissive));
        
        std::vector<Primitive*> meshprims;
        lightmesh->Refine(meshprims);
        
        // Add area lights
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
    
    // World
    std::unique_ptr<World> world_;
    //  Texture system
    std::unique_ptr<TextureSystem> texsys_;
    
    // Image I/O
    OiioImageIo io_;
    
    // Comparator
    std::unique_ptr<ImgCompare> imgcmp_;
};


TEST_F(BasicFeatures, PointLight)
{
    // Test file name
    std::string testfilename = std::string() + "/" + test_info_->test_case_name() + "." + test_info_->name() + ".png";
    // Image plane
    FileImagePlane imgplane(g_output_image_path + testfilename, g_imgres, io_);
    
    // Create renderer
    MtImageRenderer imgrenderer(
                        imgplane, // Image plane
                        new DiTracer(), // Tracer
                        new RegularSampler(g_num_spp), // Image sampler
                        new RegularSampler(1), // Light sampler
                        new RegularSampler(1) // Brdf sampler
                        );
    
    // Override world settings: we are testning point light
    world_->lights_.clear();
    
    // Simple point light
    PointLight* light = new PointLight(float3(0.f, 5.f, 0.f), float3(20.f,18.f,14.f));
    
    // Add our point light
    world_->lights_.push_back(std::unique_ptr<PointLight>(light));
    
    // Start testing
    ASSERT_NO_THROW(imgrenderer.Render(*world_));
    
    // Compare
    if (g_compare)
    {
        ImgCompare::Statistics stat;
        imgcmp_->Compare(g_ref_image_path + testfilename, g_output_image_path + testfilename, stat);
    
        ASSERT_EQ(stat.sizediff, false);
        ASSERT_EQ(stat.ndiff, 0);
    }
}

TEST_F(BasicFeatures, DirectionalLight)
{
    // Test file name
    std::string testfilename = std::string() + "/" + test_info_->test_case_name() + "." + test_info_->name() + ".png";
    // Image plane
    FileImagePlane imgplane(g_output_image_path + testfilename, g_imgres, io_);
    
    // Create renderer
    MtImageRenderer imgrenderer(
                                imgplane, // Image plane
                                new DiTracer(), // Tracer
                                new RegularSampler(g_num_spp), // Image sampler
                                new RegularSampler(1), // Light sampler
                                new RegularSampler(1) // Brdf sampler
                                );
    
    // Override world settings: we are testning point light
    world_->lights_.clear();
    
    // Simple directional light
    DirectionalLight* light = new DirectionalLight(float3(-1.f, -1.f, 1.f), float3(2.f, 1.8f, 1.4f));
    
    // Add our point light
    world_->lights_.push_back(std::unique_ptr<DirectionalLight>(light));
    
    // Start testing
    ASSERT_NO_THROW(imgrenderer.Render(*world_));
    
    // Compare
    if (g_compare)
    {
        ImgCompare::Statistics stat;
        imgcmp_->Compare(g_ref_image_path + testfilename, g_output_image_path + testfilename, stat);
        
        ASSERT_EQ(stat.sizediff, false);
        ASSERT_EQ(stat.ndiff, 0);
    }
}

TEST_F(BasicFeatures, EnvironmentLight)
{
    // Test file name
    std::string testfilename = std::string() + "/" + test_info_->test_case_name() + "." + test_info_->name() + ".png";
    // Image plane
    FileImagePlane imgplane(g_output_image_path + testfilename, g_imgres, io_);
    
    // Create renderer
    MtImageRenderer imgrenderer(
                                imgplane, // Image plane
                                new DiTracer(), // Tracer
                                new RegularSampler(g_num_spp), // Image sampler
                                new StratifiedSampler(1, new McRng()), // Light sampler
                                new StratifiedSampler(1, new McRng()) // Brdf sampler
                                );
    
    // Override world settings: we are testning point light
    world_->lights_.clear();
    
    // Simple directional light
    EnvironmentLight* light = new EnvironmentLight(*texsys_, "Apartment.hdr", 0.6f);
    
    // Add our point light
    world_->lights_.push_back(std::unique_ptr<EnvironmentLight>(light));
    
    // Start testing
    ASSERT_NO_THROW(imgrenderer.Render(*world_));
    
    // Compare
    if (g_compare)
    {
        ImgCompare::Statistics stat;
        imgcmp_->Compare(g_ref_image_path + testfilename, g_output_image_path + testfilename, stat);
        
        ASSERT_EQ(stat.sizediff, false);
        ASSERT_EQ(stat.ndiff, 0);
    }
}

TEST_F(BasicFeatures, AreaLight)
{
    // Test file name
    std::string testfilename = std::string() + "/" + test_info_->test_case_name() + "." + test_info_->name() + ".png";
    // Image plane
    FileImagePlane imgplane(g_output_image_path + testfilename, g_imgres, io_);
    
    // Create renderer
    MtImageRenderer imgrenderer(
                                imgplane, // Image plane
                                new DiTracer(), // Tracer
                                new RegularSampler(g_num_spp), // Image sampler
                                new StratifiedSampler(1, new McRng()), // Light sampler
                                new StratifiedSampler(1, new McRng()) // Brdf sampler
                                );
    
    // Override world settings: we are testning point light
    world_ = BuildWorldAreaLight();
    
    // Start testing
    ASSERT_NO_THROW(imgrenderer.Render(*world_));
    
    // Compare
    if (g_compare)
    {
        ImgCompare::Statistics stat;
        imgcmp_->Compare(g_ref_image_path + testfilename, g_output_image_path + testfilename, stat);
        
        ASSERT_EQ(stat.sizediff, false);
        ASSERT_EQ(stat.ndiff, 0);
    }
}

TEST_F(BasicFeatures, PerspectiveCamera)
{
    // Test file name
    std::string testfilename = std::string() + "/" + test_info_->test_case_name() + "." + test_info_->name() + ".png";
    // Image plane
    FileImagePlane imgplane(g_output_image_path + testfilename, g_imgres, io_);
    
    // Create renderer
    MtImageRenderer imgrenderer(
                                imgplane, // Image plane
                                new DiTracer(), // Tracer
                                new RegularSampler(g_num_spp), // Image sampler
                                new RegularSampler(1), // Light sampler
                                new RegularSampler(1) // Brdf sampler
                                );
    
    // Override world settings: we are testning point light
    world_->camera_.reset(
                          new PerscpectiveCamera(// Eye point
                                                 float3(0.f, 2.f, -10.5f),
                                                 // At point
                                                 float3(0,0,0),
                                                 // Up vector
                                                 float3(1, 0, 0),
                                                 // Range
                                                 float2(0.01f, 10000.f),
                                                 // Vertical FOV
                                                 PI / 4,
                                                 // Aspect ratio
                                                 1.f)
                          );
    
    // Start testing
    ASSERT_NO_THROW(imgrenderer.Render(*world_));
    
    // Compare
    if (g_compare)
    {
        ImgCompare::Statistics stat;
        imgcmp_->Compare(g_ref_image_path + testfilename, g_output_image_path + testfilename, stat);
        
        ASSERT_EQ(stat.sizediff, false);
        ASSERT_EQ(stat.ndiff, 0);
    }
}

TEST_F(BasicFeatures, EnvironmentCamera)
{
    // Test file name
    std::string testfilename = std::string() + "/" + test_info_->test_case_name() + "." + test_info_->name() + ".png";
    // Image plane
    FileImagePlane imgplane(g_output_image_path + testfilename, g_imgres, io_);
    
    // Create renderer
    ImageRenderer imgrenderer(
                                imgplane, // Image plane
                                new DiTracer(), // Tracer
                                new RegularSampler(g_num_spp), // Image sampler
                                new RegularSampler(1), // Light sampler
                                new RegularSampler(1) // Brdf sampler
                                );
    
    // Override world settings: we are testning point light
    world_->camera_.reset(
                          new EnvironmentCamera(// Eye point
                                                 float3(0.f, 2.f, -3.5f),
                                                 // At point
                                                 float3(0,0,0),
                                                 // Up vector
                                                 float3(0, 1, 0),
                                                 // Range
                                                 float2(0.01f, 10000.f)
                                                )
                          );
    
    // Start testing
    ASSERT_NO_THROW(imgrenderer.Render(*world_));
    
    // Compare
    if (g_compare)
    {
        ImgCompare::Statistics stat;
        imgcmp_->Compare(g_ref_image_path + testfilename, g_output_image_path + testfilename, stat);
        
        ASSERT_EQ(stat.sizediff, false);
        ASSERT_EQ(stat.ndiff, 0);
    }
}


#endif // FIRERAYS_TEST_H
