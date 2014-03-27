//
//  CLWParallelPrimitives.cpp
//  CLW
//
//  Created by dmitryk on 11.03.14.
//  Copyright (c) 2014 dmitryk. All rights reserved.
//

#include "CLWParallelPrimitives.h"

#include <cassert>
#include <string>
#include <vector>
#include <fstream>

#define WG_SIZE 64
#define NUM_SCAN_ELEMS_PER_WI 8
#define NUM_SCAN_ELEMS_PER_WG (WG_SIZE * NUM_SCAN_ELEMS_PER_WI)

static void load_file_contents(std::string const& name, std::vector<char>& contents, bool binary)
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


CLWParallelPrimitives::CLWParallelPrimitives(CLWContext context)
    : context_(context)
{
    std::vector<char> sourceCode;
    load_file_contents("../../../CLW/CL/CLW.cl", sourceCode, false);
    program_ = CLWProgram::CreateFromSource(sourceCode, context_);
}

CLWParallelPrimitives::~CLWParallelPrimitives()
{
    ReclaimDeviceMemory();
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();

    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");

    topLevelScan.SetArg(0, input);
    topLevelScan.SetArg(1, output);
    topLevelScan.SetArg(2, (cl_uint)numElems);
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));

    return context_.Launch1D(0, WG_SIZE, WG_SIZE, topLevelScan);
}


CLWEvent CLWParallelPrimitives::ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();

    int GROUP_BLOCK_SIZE_SCAN_BOTTOM_LEVEL = (WG_SIZE * 8);
    int GROUP_BLOCK_SIZE_SCAN_TOP_LEVEL = (WG_SIZE * 8);
    int NUM_GROUPS_BOTTOM_LEVEL = (numElems + GROUP_BLOCK_SIZE_SCAN_BOTTOM_LEVEL - 1) / GROUP_BLOCK_SIZE_SCAN_BOTTOM_LEVEL;
    int NUM_GROUPS_TOP_LEVEL = (NUM_GROUPS_BOTTOM_LEVEL + GROUP_BLOCK_SIZE_SCAN_TOP_LEVEL - 1) / GROUP_BLOCK_SIZE_SCAN_TOP_LEVEL;

    auto devicePartSums = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL);
        //context_.CreateBuffer<cl_int>(NUM_GROUPS_BOTTOM_LEVEL);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_int4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_int4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSums);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSums);
    topLevelScan.SetArg(1, devicePartSums);
    topLevelScan.SetArg(2, (cl_uint)devicePartSums.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSums);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempIntBuffer(devicePartSums);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL * WG_SIZE, WG_SIZE, distributeSums);
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();

    int GROUP_BLOCK_SIZE_SCAN_BOTTOM_LEVEL = (WG_SIZE * 8);
    int GROUP_BLOCK_SIZE_SCAN_MID_LEVEL = (WG_SIZE * 8);
    int GROUP_BLOCK_SIZE_SCAN_TOP_LEVEL = (WG_SIZE * 8);
    int NUM_GROUPS_BOTTOM_LEVEL = (numElems + GROUP_BLOCK_SIZE_SCAN_BOTTOM_LEVEL - 1) / GROUP_BLOCK_SIZE_SCAN_BOTTOM_LEVEL;
    int NUM_GROUPS_MID_LEVEL = (NUM_GROUPS_BOTTOM_LEVEL + GROUP_BLOCK_SIZE_SCAN_MID_LEVEL - 1) / GROUP_BLOCK_SIZE_SCAN_MID_LEVEL;
    int NUM_GROUPS_TOP_LEVEL = (NUM_GROUPS_MID_LEVEL + GROUP_BLOCK_SIZE_SCAN_TOP_LEVEL - 1) / GROUP_BLOCK_SIZE_SCAN_TOP_LEVEL;

    auto devicePartSumsBottomLevel = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL);
    auto devicePartSumsMidLevel = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_int4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_int4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL * WG_SIZE, WG_SIZE, bottomLevelScan);

    bottomLevelScan.SetArg(0, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(1, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    bottomLevelScan.SetArg(3, devicePartSumsMidLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSumsMidLevel);
    topLevelScan.SetArg(1, devicePartSumsMidLevel);
    topLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSumsMidLevel);
    distributeSums.SetArg(1, devicePartSumsBottomLevel);
    distributeSums.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL * WG_SIZE, WG_SIZE, distributeSums);

    distributeSums.SetArg(0, devicePartSumsBottomLevel);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempIntBuffer(devicePartSumsMidLevel);
    ReclaimTempIntBuffer(devicePartSumsBottomLevel);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL * WG_SIZE, WG_SIZE, distributeSums);
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output)
{
    assert(input.GetElementCount() == output.GetElementCount());

    cl_uint numElems = (cl_uint)input.GetElementCount();

    if (numElems < NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddWG(deviceIdx, input, output);
    }
    else if (numElems < NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddTwoLevel(deviceIdx, input, output);
    }
    else if (numElems < NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddThreeLevel(deviceIdx, input, output);
    }
    else
    {
        assert(0);
    }

    return CLWEvent::Create(nullptr);
}

CLWBuffer<cl_int> CLWParallelPrimitives::GetTempIntBuffer(size_t size)
{
    auto iter = intBufferCache_.find(size);

    if (iter != intBufferCache_.end())
    {
        CLWBuffer<int> tmp = iter->second;
        intBufferCache_.erase(iter);
        return tmp;
    }

    return context_.CreateBuffer<cl_int>(size);
}

CLWEvent CLWParallelPrimitives::SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys,
                                               CLWBuffer<cl_int> inputValues, CLWBuffer<cl_int> outputValues)
{
    assert(inputKeys.GetElementCount() == outputKeys.GetElementCount());

    cl_uint numElems = (cl_uint)inputKeys.GetElementCount();

    int GROUP_BLOCK_SIZE = (WG_SIZE * 4);
    int NUM_BLOCKS =  (numElems + GROUP_BLOCK_SIZE - 1)/ GROUP_BLOCK_SIZE;
        
    auto deviceHistograms = GetTempIntBuffer(NUM_BLOCKS * 4);
    auto deviceLocalOffsets = GetTempIntBuffer(numElems);

    auto fromKeys = &inputKeys;
    auto toKeys = &outputKeys;
    
    auto fromVals = &inputValues;
    auto toVals = &outputValues;

    CLWKernel splitKernel = program_.GetKernel("split_4way");
    CLWKernel scatterKeys = program_.GetKernel("scatter_keys");

    for (int offset = 0; offset < 32; offset += 2)
    {
        // Split 
        splitKernel.SetArg(0, offset);
        splitKernel.SetArg(1, *fromKeys);
        splitKernel.SetArg(2, (cl_uint)fromKeys->GetElementCount());
        splitKernel.SetArg(3, deviceHistograms);
        splitKernel.SetArg(4, deviceLocalOffsets);
        splitKernel.SetArg(5, SharedMemory(WG_SIZE * 4 * sizeof(cl_short)));
        context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, splitKernel);

        // Scan histograms
        ScanExclusiveAdd(0, deviceHistograms, deviceHistograms);

        // Scatter keys
        scatterKeys.SetArg(0, offset);
        scatterKeys.SetArg(1, *fromKeys);
        scatterKeys.SetArg(2, *fromVals);
        scatterKeys.SetArg(3, (cl_uint)toKeys->GetElementCount());
        scatterKeys.SetArg(4, deviceHistograms);
        scatterKeys.SetArg(5, deviceLocalOffsets);
        scatterKeys.SetArg(6, *toKeys);
        scatterKeys.SetArg(7, *toVals);
        context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, scatterKeys);

        // Swap pointers
        std::swap(fromKeys, toKeys);
        std::swap(fromVals, toVals);
        
    }

    // Return buffers to memory manager
    ReclaimTempIntBuffer(deviceHistograms);
    ReclaimTempIntBuffer(deviceLocalOffsets);

    // Return last copy event back to the user
    context_.CopyBuffer(0, *fromKeys, *toKeys, 0, 0, numElems);
    return context_.CopyBuffer(0, *fromVals, *toVals, 0, 0, numElems);
}

void CLWParallelPrimitives::ReclaimDeviceMemory()
{
    intBufferCache_.clear();
}

void CLWParallelPrimitives::ReclaimTempIntBuffer(CLWBuffer<cl_int> buffer)
{
    auto iter = intBufferCache_.find(buffer.GetElementCount());

    if (iter != intBufferCache_.end())
    {
        return;
    }

    intBufferCache_[buffer.GetElementCount()] = buffer;
}
