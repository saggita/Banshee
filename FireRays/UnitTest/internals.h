#ifndef INTERNALS_H
#define INTERNALS_H

/// Some internal functionality tests here

#include <gtest/gtest.h>

#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "math/mathutils.h"
#include "math/distribution1d.h"
#include "math/distribution2d.h"

extern std::string g_output_image_path;
extern std::string g_ref_image_path;
extern std::string g_texture_path;
extern int2 g_imgres;
extern bool g_compare;
extern int  g_num_spp;

class Internals : public ::testing::Test
{
public:
    virtual void SetUp()
    {
    
    }
    
    virtual void TearDown()
    {
        
    }
};


///< Test 1D piecewise constant distribution RNG
TEST_F(Internals, Distribution1D)
{
    static int kNumSamples = 10000;
    float pdf[] = {0.1f, 0.9f, 0.1f, 0.1f};
    
    Distribution1D dist(4, pdf);
    
    int bins[4] = {0, 0, 0, 0};
    float vpdf = 0.f;
    
    for (int i = 0; i < kNumSamples; ++i)
    {
        float v = dist.Sample1D(rand_float(), vpdf);
        
        if (v <= 0.25f)
        {
            bins[0]++;
        }
        else if (v <= 0.5f)
        {
            bins[1]++;
        }
        else if (v <= 0.75f)
        {
            bins[2]++;
        }
        else
        {
            bins[3]++;
        }
    }
    
    ASSERT_GT(bins[1], bins[0]);
    ASSERT_GT(bins[1], bins[2]);
    ASSERT_GT(bins[1], bins[3]);
}

///< Test 1D piecewise constant distribution RNG
TEST_F(Internals, Distribution2D)
{
    static int kNumSamples = 10000;
    float pdf[] = {0.2f, 0.2f, 0.9f, 0.0f};
    
    Distribution2D dist(2,2,pdf);
    
    int bins[2][2] = {0, 0, 0, 0};
    float vpdf = 0.f;
    
    for (int i = 0; i < kNumSamples; ++i)
    {
        float2 v = dist.Sample2D(float2(rand_float(), rand_float()), vpdf);
        
        int x = clamp((int)(v.x/0.5), 0, 1);
        int y = clamp((int)(v.y/0.5), 0, 1);
        bins[y][x]++;
    }
    
    ASSERT_GT(bins[1][0], bins[0][0]);
    ASSERT_GT(bins[1][0], bins[0][1]);
    ASSERT_GT(bins[1][0], bins[1][1]);
}



#endif // INTERNALS_H