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


#include "BVH.h"
#include "SplitBVHBuilder.h"
#include "OCLBVHBackEnd.h"
#include "TestScene.h"

int main(int argc, const char * argv[])
{
    auto scene  = TestScene::Create();
    
    BVH bvh;
    SplitBVHBuilder builder(scene->GetVertices(), scene->GetVertexCount(), scene->GetIndices(), scene->GetIndexCount(), scene->GetMaterials(), 8U, 8U, 1.f, 1.f);
    
	static auto prevTime = std::chrono::high_resolution_clock::now();
    
    //std::shared_ptr<BVHAccelerator> accel = BVHAccelerator::CreateFromScene(*scene);
	builder.SetBVH(&bvh);
    builder.Build();
    
    OCLBVHBackEnd oclBackEnd(bvh);
    
    oclBackEnd.Generate();
    
    auto currentTime = std::chrono::high_resolution_clock::now();
	auto deltaTime   = std::chrono::duration_cast<std::chrono::duration<double> >(currentTime - prevTime);
    
    std::cout << "\nBuilding time " << deltaTime.count() << " secs\n";
    
    //BVH& bvh(accel->bvh_);
    
	return 0;
}


