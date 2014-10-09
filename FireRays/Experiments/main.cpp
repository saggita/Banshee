//
//  main.cpp
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//
#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <iterator>
#include <chrono>

#include "CLW.h"

#include "BVH.h"
#include "SplitBVHBuilder.h"
#include "LinearBVHBuilder.h"
#include "OCLBVHBackEnd.h"
#include "TestScene.h"

void test_1()
{

    auto scene  = TestScene::Create();

    BVH bvh;
    //SplitBVHBuilder builder(scene->GetVertices(), scene->GetVertexCount(), scene->GetIndices(), scene->GetIndexCount(), scene->GetMaterials(), 32U, 1.f, 1.f);

    LinearBVHBuilder builder(scene->GetVertices(), scene->GetVertexCount(), scene->GetIndices(), scene->GetIndexCount(), scene->GetMaterials());

    static auto prevTime = std::chrono::high_resolution_clock::now();

    builder.SetBVH(&bvh);
    builder.Build();

    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime   = std::chrono::duration_cast<std::chrono::duration<double> >(currentTime - prevTime);

    std::cout << "\nBuilding time " << deltaTime.count() << " secs\n";

    // generate points
    BVH::Iterator* iter       = bvh.CreateDepthFirstIterator();
    BBox           rootBounds = bvh.GetNodeBbox(iter->GetNodeId());


    std::vector<vector3opt> points;
    float xrange = rootBounds.GetExtents().x * 2;
    float yrange = rootBounds.GetExtents().y * 2;
    float zrange = rootBounds.GetExtents().z * 2;

    const int NUM_POINTS = 100000;
    for (int i = 0; i < NUM_POINTS; ++i)
    {
        float x = ((float)rand() / RAND_MAX) * xrange;
        float y = ((float)rand() / RAND_MAX) * yrange;
        float z = ((float)rand() / RAND_MAX) * zrange;

        vector3opt p = rootBounds.min - rootBounds.GetExtents() * 0.5f;
        p += vector3opt(x,y,z);
        points.push_back(p);
    }

    BVH::RayQueryStatistics globalStat;

    globalStat.maxDepthVisited = 0;
    globalStat.numNodesVisited = 0;
    globalStat.numTrianglesTested = 0;

    int globalRayHits = 0;
    int globalRayLeafHits = 0;

    const int NUM_RAYS = 100000;
    for (int i = 0; i < NUM_RAYS; ++i)
    {
        int idx1 = rand() % points.size();
        int idx2 = rand() % points.size();

        BVH::RayQuery q;
        q.o = points[idx1];
        q.d = (points[idx2] - points[idx1]).normalize();
        q.t = std::numeric_limits<float>::max();

        BVH::RayQueryStatistics stat;
        bvh.CastRay(q, stat, scene->GetVertices(), scene->GetIndices());

        if (stat.hitBvh)
        {
            globalStat.numNodesVisited += stat.numNodesVisited;
            globalStat.numTrianglesTested += stat.numTrianglesTested;
            globalStat.maxDepthVisited += stat.maxDepthVisited;
            globalRayHits++;

            if (stat.numTrianglesTested > 0)
            {
                globalRayLeafHits++;
            }
        }
    }

    std::cout << "-------\n";
    std::cout << "BVH node count " << bvh.GetNodeCount() << "\n";
    std::cout << "Rays emitted: " << NUM_RAYS << "\n";
    std::cout << "Rays hit BVH root: " << globalRayHits << "\n";
    std::cout << "Rays hit BVH leaf: " << globalRayLeafHits << "\n";
    std::cout << "Avg nodes visited per ray: " << ((float)globalStat.numNodesVisited) / globalRayHits << "\n";
    std::cout << "Avg triangles tested per ray: " << ((float)globalStat.numTrianglesTested) / globalRayHits << "\n";
    std::cout << "Avg max depth visited per ray: " << ((float)globalStat.maxDepthVisited) / globalRayHits << "\n";

}


void scan_test(CLWContext context)
{
    int const ARRAY_SIZE = 100000000;
    
    auto deviceInputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);
    auto deviceOutputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);
    
    std::vector<int> hostArray(ARRAY_SIZE);
    std::vector<int> hostArrayGold(ARRAY_SIZE);
    
    std::generate(hostArray.begin(), hostArray.end(), []{return rand() % 1000; });
    
    int sum = 0;
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        hostArrayGold[i] = sum;
        sum += hostArray[i];
    }
    
    context.WriteBuffer(0, deviceInputArray, &hostArray[0], ARRAY_SIZE).Wait();

    CLWParallelPrimitives prims(context);

    prims.ScanExclusiveAdd(0, deviceInputArray, deviceOutputArray);
    context.Finish(0);

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 50; ++i)
        prims.ScanExclusiveAdd(0, deviceInputArray, deviceOutputArray);

    context.Finish(0);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    context.ReadBuffer(0, deviceOutputArray, &hostArray[0], ARRAY_SIZE).Wait();

    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        if (hostArray[i] != hostArrayGold[i])
        {
            std::cout << "Incorrect scan result\n";
            std::cout << "Done in " << deltaTime.count() / 50.f << " ms\n";
            exit(-1);
        }
    }
    
    std::cout << "Correct scan result\n";
    std::cout << "Done in " << deltaTime.count() / 50.f << " ms\n";
}

void segmented_scan_test(CLWContext context)
{
    int const ARRAY_SIZE = 64 * 64 * 64;
    
    auto deviceInputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);
    auto deviceOutputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);
    auto deviceSegmentHeadsArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_ONLY);
    
    std::vector<int> hostArray(ARRAY_SIZE);
    std::vector<int> hostResultArray(ARRAY_SIZE);
    std::vector<int> hostSegmentHeadsArray(ARRAY_SIZE);
    std::vector<int> hostArrayGold(ARRAY_SIZE);
    
    
    std::generate(hostArray.begin(), hostArray.end(), []{return rand() % 100; });
    std::generate(hostSegmentHeadsArray.begin(), hostSegmentHeadsArray.end(), []{return rand() % 2;});
    
    int sum = 0;
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        if (hostSegmentHeadsArray[i])
            sum = 0;
        
        hostArrayGold[i] = sum;
        sum += hostArray[i];
    }
    
    context.WriteBuffer(0, deviceInputArray, &hostArray[0], ARRAY_SIZE).Wait();
    context.WriteBuffer(0, deviceSegmentHeadsArray, &hostSegmentHeadsArray[0], ARRAY_SIZE).Wait();
    
    CLWParallelPrimitives prims(context);
    
    prims.SegmentedScanExclusiveAdd(0, deviceInputArray, deviceSegmentHeadsArray, deviceOutputArray);
    context.Finish(0);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 50; ++i)
    {
        prims.SegmentedScanExclusiveAdd(0, deviceInputArray, deviceSegmentHeadsArray, deviceOutputArray);
    }
    
    context.Finish(0);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    context.ReadBuffer(0, deviceOutputArray, &hostResultArray[0], ARRAY_SIZE).Wait();
    
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        if (hostResultArray[i] != hostArrayGold[i])
        {
            std::cout << "Incorrect scan result\n";
            std::cout << "Done in " << deltaTime.count() / 50.f << " ms\n";
            exit(-1);
        }
    }
    
    std::cout << "Correct scan result\n";
    std::cout << "Done in " << deltaTime.count() / 50.f << " ms\n";
}

void sort_test(CLWContext context)
{
    int const ARRAY_SIZE = 10000000;
    
    auto deviceInputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);
    auto deviceOutputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);

    std::vector<int> hostArray(ARRAY_SIZE);
    std::vector<int> hostResultArray(ARRAY_SIZE);
    std::vector<int> hostArrayGold(ARRAY_SIZE);
    
    
    std::generate(hostArray.begin(), hostArray.end(), []{return rand() % 16;});
    std::copy(hostArray.begin(), hostArray.end(), hostArrayGold.begin());
    std::sort(hostArrayGold.begin(), hostArrayGold.end());

    context.WriteBuffer(0, deviceInputArray, &hostArray[0], ARRAY_SIZE).Wait();

    CLWParallelPrimitives prims(context);

    prims.SortRadix(0, deviceInputArray, deviceOutputArray);
    context.Finish(0);

    auto startTime = std::chrono::high_resolution_clock::now();

    //for (int i = 0; i < 50; ++i)
    //{
        //prims.SortRadix(0, deviceInputArray, deviceOutputArray);
    //}

    context.Finish(0);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    context.ReadBuffer(0, deviceOutputArray, &hostResultArray[0], ARRAY_SIZE).Wait();

    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        if (hostResultArray[i] != hostArrayGold[i])
        {
            std::cout << "Incorrect sort result\n";
            std::cout << "Done in " << deltaTime.count() / 50.f << " ms\n";
            exit(-1);
        }
    }
    
    std::cout << "Correct sort result\n";
    std::cout << "Done in " << deltaTime.count() / 50.f << " ms\n";
}


void copy_test(CLWContext context)
{
    int const ARRAY_SIZE = 100000000;
    
    auto deviceInputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);
    auto deviceOutputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE, CL_MEM_READ_WRITE);
    
    std::vector<int> hostArray(ARRAY_SIZE);
    std::vector<int> hostOutputArray(ARRAY_SIZE);

    std::generate(hostArray.begin(), hostArray.end(), []{return rand() % 1000; });

    context.WriteBuffer(0, deviceInputArray, &hostArray[0], ARRAY_SIZE).Wait();

    CLWParallelPrimitives prims(context);

    context.Finish(0);

    prims.Copy(0, deviceInputArray, deviceOutputArray);

    auto startTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 50; ++i)
    {
        prims.Copy(0, deviceInputArray, deviceOutputArray);
    }

    context.Finish(0);

    auto endTime = std::chrono::high_resolution_clock::now();

    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    context.ReadBuffer(0, deviceOutputArray, &hostOutputArray[0], ARRAY_SIZE).Wait();

    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        if (hostArray[i] != hostOutputArray[i])
        {
            std::cout << "Incorrect copy result\n";
            exit(-1);
        }
    }
    
    std::cout << "Correct copy result\n";
    std::cout << "Done in " << deltaTime.count() / 50.f << " ms\n";
}



int main(int argc, const char * argv[])
{
    try
    {
        std::vector<CLWPlatform> platforms;

        CLWPlatform::CreateAllPlatforms(platforms);

        CLWContext context;
        bool       bAmdContext = false;
        for (int i = 0; i < platforms.size(); ++i)
        {
            if (platforms[i].GetVendor().find("AMD") != std::string::npos ||
                platforms[i].GetVendor().find("Advanced Micro Devices") != std::string::npos)
            {
                context = CLWContext::Create(platforms[i].GetDevice(0));
                std::cout << "Platform in use: " << platforms[i].GetName() << "\n";
                std::cout << "OpenCL version: " << platforms[i].GetVersion() << "\n";
                bAmdContext = true;
            }
        }

        if (!bAmdContext)
        {
            context = CLWContext::Create(platforms[0].GetDevice(1));
        }

        std::cout << "Device in use: " << context.GetDevice(0).GetName() << "\n";
        std::cout << "Extensions: " << context.GetDevice(0).GetExtensions() << "\n";

        //scan_test(context);
        //segmented_scan_test(context);

        sort_test(context);
        //test_1();
        std::cout << "Available memory: " << context.GetDevice(0).GetGlobalMemSize() / (1024 * 1024) << "Mb\n";
        std::cout << "Max memory allocation size: " << context.GetDevice(0).GetMaxAllocSize() / (1024 * 1024) << "Mb\n";

        // Test for allocation 8Gb
        //auto buffer = context.CreateBuffer<int>(1.5 * 2147483648, CL_MEM_READ_WRITE, nullptr);

    }
    catch (CLWExcept& e)
    {
        std::cout << "\n" << e.what() << "\n";
    }
    catch (std::exception& e)
    {
        std::cout << "\n" << e.what() << "\n";
    }

    return 0;
}


