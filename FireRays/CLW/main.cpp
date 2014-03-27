//
//  main.cpp
//  CLW
//
//  Created by dmitryk on 30.11.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//
#include "CLW.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

void load_file_contents(std::string const& name, std::vector<char>& contents, bool binary)
{
	std::ifstream in(name, std::ios::in | (binary?std::ios::binary : 0));
    
	if (in)
	{
		contents.clear();
        
		std::streamoff beg = in.tellg();
        
		in.seekg(0, std::ios::end);
        
		std::streamoff fileSize = in.tellg() - beg;
        
		in.seekg(0, std::ios::beg);
        
		contents.resize(static_cast<unsigned>(fileSize));
        
		in.read(&contents[0], fileSize);
	}
	else
	{
		throw std::runtime_error("Cannot read the contents of a file");
	}
}

void sort_test(CLWContext context)
{
    int const ARRAY_SIZE = 7942;
    
    
    auto deviceArray = context.CreateBuffer<cl_int>(ARRAY_SIZE);
    auto deviceArray1 = context.CreateBuffer<cl_int>(ARRAY_SIZE);
    auto deviceValuesArray = context.CreateBuffer<cl_int>(ARRAY_SIZE);
    auto deviceValuesArray1 = context.CreateBuffer<cl_int>(ARRAY_SIZE);
    
    std::vector<int> hostArray(ARRAY_SIZE);
    std::vector<int> hostValuesArray(ARRAY_SIZE);
    std::generate(hostArray.begin(), hostArray.end(), []{return rand() % 1000; });
    
    for(int i=0 ; i<ARRAY_SIZE; ++i)
    {
        hostValuesArray[i] = i;
    }
    
    std::vector<int> hostArrayGold(hostArray.begin(), hostArray.end());
    std::sort(hostArrayGold.begin(), hostArrayGold.end());
    
    context.WriteBuffer(0, deviceArray, &hostArray[0], ARRAY_SIZE).Wait();
    context.WriteBuffer(0, deviceValuesArray, &hostValuesArray[0], ARRAY_SIZE).Wait();
    
    CLWParallelPrimitives prims(context);
    
    prims.SortRadix(0, deviceArray, deviceArray1, deviceValuesArray, deviceValuesArray1);
    
    //context.ReadBuffer(0, deviceArray1, &hostArray[0], ARRAY_SIZE).Wait();
    context.ReadBuffer(0, deviceValuesArray1, &hostValuesArray[0], ARRAY_SIZE).Wait();
    
    // reorder to check key-value sort correctness
    std::vector<int> tempArray(ARRAY_SIZE);
    for (int i=0;i<ARRAY_SIZE;++i)
        tempArray[i] = hostArray[hostValuesArray[i]];
    
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        if (tempArray[i] != hostArrayGold[i])
        {
            std::cout << "Incorrect sorting result\n";
            exit(-1);
        }
    }
    
    std::cout << "Correct sorting result\n";
}


void scan_test(CLWContext context)
{
    int const ARRAY_SIZE = 16444953;
    
    auto deviceInputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE);
    auto deviceOutputArray = context.CreateBuffer<cl_int>(ARRAY_SIZE);
    
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
    
    context.ReadBuffer(0, deviceOutputArray, &hostArray[0], ARRAY_SIZE).Wait();
    
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        if (hostArray[i] != hostArrayGold[i])
        {
            std::cout << "Incorrect scan result\n";
            exit(-1);
        }
    }
    
    std::cout << "Correct scan result\n";
}

int main(int argc, const char * argv[])
{
    try
    {
        std::vector<CLWPlatform> platforms;
        CLWPlatform::CreateAllPlatforms(platforms);
        auto platform = platforms[0];
        platforms.clear();
        
        std::cout << "Platform vendor: " << platform.GetVendor() << "\n";
        std::cout << "Platform name: " << platform.GetName() << "\n";
        std::cout << "Platform profile: " << platform.GetProfile() << "\n";
        std::cout << "Platform version: " << platform.GetVersion() << "\n";
        std::cout << "Platform extensions: " << platform.GetExtensions() << "\n";
        
        std::cout << "Number of devices:" << platform.GetDeviceCount() << "\n";
        std::cout << "-----DEVICE LIST-----\n";
        
        for (unsigned i = 0; i < platform.GetDeviceCount(); ++i)
        {
            std::cout << "Device name: " << platform.GetDevice(i).GetName() << "\n";
            std::cout << "Device vendor: " << platform.GetDevice(i).GetVendor() << "\n";
            std::cout << "Device version: " << platform.GetDevice(i).GetVersion() << "\n";
            std::cout << "Device profile: " << platform.GetDevice(i).GetProfile() << "\n";
            std::cout << "Device global mem size: " << platform.GetDevice(i).GetGlobalMemSize() << "\n";
            std::cout << "Device local mem size: " << platform.GetDevice(i).GetLocalMemSize() << "\n";
            std::cout << "Device max WG size: " << platform.GetDevice(i).GetMaxWorkGroupSize() << "\n";
            
            std::string type;
            switch (platform.GetDevice(i).GetType())
            {
                case CL_DEVICE_TYPE_CPU:
                    type = "CPU";
                    break;
                    
                case CL_DEVICE_TYPE_GPU:
                    type = "GPU";
                    break;
                    
                case CL_DEVICE_TYPE_ACCELERATOR:
                    type = "Accelerator";
                    break;
                    
                default:
                    break;
            }
            std::cout << "Device type: " << type << "\n";
            //std::cout << "Device extensions: " << platform.GetDevice(i).GetExtensions() << "\n";
            std::cout << "---->\n";
        }
        
        auto context = CLWContext::Create(platform.GetDevice(1));
        
        scan_test(context);
        sort_test(context);
        
    }
    catch(std::runtime_error& e)
    {
        std::cout << e.what() << "\n";
        return -1;
    }
    
    return 0;
}

