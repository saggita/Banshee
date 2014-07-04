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
#define NUM_SEG_SCAN_ELEMS_PER_WI 1
#define NUM_SCAN_ELEMS_PER_WG (WG_SIZE * NUM_SCAN_ELEMS_PER_WI)
#define NUM_SEG_SCAN_ELEMS_PER_WG (WG_SIZE * NUM_SEG_SCAN_ELEMS_PER_WI)

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

    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSums = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
        //context_.CreateBuffer<cl_int>(NUM_GROUPS_BOTTOM_LEVEL);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_int4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_int4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSums);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSums);
    topLevelScan.SetArg(1, devicePartSums);
    topLevelScan.SetArg(2, (cl_uint)devicePartSums.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSums);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempIntBuffer(devicePartSums);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}




CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE);
    
    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    
    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    
    auto devicePartSums = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartFlags = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    //context_.CreateBuffer<cl_int>(NUM_GROUPS_BOTTOM_LEVEL);
    
    CLWKernel bottomLevelScan = program_.GetKernel("segmented_scan_exclusive_int_part");
    CLWKernel topLevelScan = program_.GetKernel("segmented_scan_exclusive_int");
    CLWKernel distributeSums = program_.GetKernel("segmented_distribute_part_sum_int");
    
    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, inputHeads);
    bottomLevelScan.SetArg(2, output);
    //bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSums);
    bottomLevelScan.SetArg(4, devicePartFlags);
    bottomLevelScan.SetArg(5, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);
    
    
    
    std::vector<cl_int> hostPartSums(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    std::vector<cl_int> hostPartFlags(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    std::vector<cl_int> hostResult(numElems);
    
    context_.ReadBuffer(0,  output, &hostResult[0], numElems).Wait();
    
    context_.ReadBuffer(0,  devicePartSums, &hostPartSums[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    context_.ReadBuffer(0,  devicePartFlags, &hostPartFlags[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    
    topLevelScan.SetArg(0, devicePartSums);
    topLevelScan.SetArg(1, devicePartFlags);
    topLevelScan.SetArg(2, devicePartSums);
    //topLevelScan.SetArg(2, (cl_uint)devicePartSums.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);
    
    context_.ReadBuffer(0,  devicePartSums, &hostPartSums[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    
    distributeSums.SetArg(0, output);
    distributeSums.SetArg(1, inputHeads);
    distributeSums.SetArg(2, devicePartSums);
    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, distributeSums);
    
    //context_.ReadBuffer(0,  output, &hostResult[0], numElems).Wait();
    
    //return CLWEvent();
    //distributeSums.SetArg(0, devicePartSums);
    //distributeSums.SetArg(1, output);
    //distributeSums.SetArg(2, (cl_uint)numElems);
    
    //ReclaimTempIntBuffer(devicePartSums);
    //ReclaimTempIntBuffer(devicePartFlags);
    
    //return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}




CLWEvent CLWParallelPrimitives::ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();

    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_MID_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_MID_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    int NUM_GROUPS_MID_LEVEL_DISTRIBUTE = (NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSumsBottomLevel = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartSumsMidLevel = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_int4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_int4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    bottomLevelScan.SetArg(0, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(1, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    bottomLevelScan.SetArg(3, devicePartSumsMidLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSumsMidLevel);
    topLevelScan.SetArg(1, devicePartSumsMidLevel);
    topLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSumsMidLevel);
    distributeSums.SetArg(1, devicePartSumsBottomLevel);
    distributeSums.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);

    distributeSums.SetArg(0, devicePartSumsBottomLevel);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempIntBuffer(devicePartSumsMidLevel);
    ReclaimTempIntBuffer(devicePartSumsBottomLevel);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}



CLWEvent CLWParallelPrimitives::ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();

    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_float4");

    topLevelScan.SetArg(0, input);
    topLevelScan.SetArg(1, output);
    topLevelScan.SetArg(2, (cl_uint)numElems);
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));

    return context_.Launch1D(0, WG_SIZE, WG_SIZE, topLevelScan);
}

CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    CLWKernel topLevelScan = program_.GetKernel("segmented_scan_exclusive_int");
    
    topLevelScan.SetArg(0, input);
    topLevelScan.SetArg(1, inputHeads);
    topLevelScan.SetArg(2, output);
    //topLevelScan.SetArg(3, (cl_uint)numElems);
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    
    return context_.Launch1D(0, WG_SIZE, WG_SIZE, topLevelScan);
}


CLWEvent CLWParallelPrimitives::ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();

    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSums = GetTempFloatBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
        //context_.CreateBuffer<cl_int>(NUM_GROUPS_BOTTOM_LEVEL);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_float4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_float4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_float4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSums);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSums);
    topLevelScan.SetArg(1, devicePartSums);
    topLevelScan.SetArg(2, (cl_uint)devicePartSums.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSums);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempFloatBuffer(devicePartSums);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output)
{
cl_uint numElems = (cl_uint)input.GetElementCount();

    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_MID_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_MID_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    int NUM_GROUPS_MID_LEVEL_DISTRIBUTE = (NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSumsBottomLevel = GetTempFloatBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartSumsMidLevel = GetTempFloatBuffer(NUM_GROUPS_MID_LEVEL_SCAN);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_float4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_float4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_float4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    bottomLevelScan.SetArg(0, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(1, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    bottomLevelScan.SetArg(3, devicePartSumsMidLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSumsMidLevel);
    topLevelScan.SetArg(1, devicePartSumsMidLevel);
    topLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSumsMidLevel);
    distributeSums.SetArg(1, devicePartSumsBottomLevel);
    distributeSums.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);

    distributeSums.SetArg(0, devicePartSumsBottomLevel);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempFloatBuffer(devicePartSumsMidLevel);
    ReclaimTempFloatBuffer(devicePartSumsBottomLevel);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
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
        throw std::runtime_error("The maximum number of elements for scan exceeded\n");
    }

    return CLWEvent::Create(nullptr);
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output)
{
    assert(input.GetElementCount() == output.GetElementCount());

    cl_uint numElems = (cl_uint)input.GetElementCount();

    if (numElems <= NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddWG(deviceIdx, input, output);
    }
    else if (numElems <= NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddTwoLevel(deviceIdx, input, output);
    }
    else if (numElems <= NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddThreeLevel(deviceIdx, input, output);
    }
    else
    {
        throw std::runtime_error("The maximum number of elements for scan exceeded\n");
    }

    return CLWEvent::Create(nullptr);
}


CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    assert(input.GetElementCount() == output.GetElementCount());
    
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    if (numElems <= NUM_SEG_SCAN_ELEMS_PER_WG)
    {
        return SegmentedScanExclusiveAddWG(deviceIdx, input, inputHeads, output);
    }
    else if (numElems <= NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG)
    {
        return SegmentedScanExclusiveAddTwoLevel(deviceIdx, input, inputHeads, output);
    }
    else if (numElems <= NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG)
    {
        throw std::runtime_error("The maximum number of elements for scan exceeded\n");
        //return ScanExclusiveAddThreeLevel(deviceIdx, input, output);
    }
    else
    {
        throw std::runtime_error("The maximum number of elements for scan exceeded\n");
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

    return context_.CreateBuffer<cl_int>(size, CL_MEM_READ_WRITE);
}

CLWBuffer<cl_float> CLWParallelPrimitives::GetTempFloatBuffer(size_t size)
{
    auto iter = floatBufferCache_.find(size);

    if (iter != floatBufferCache_.end())
    {
        CLWBuffer<cl_float> tmp = iter->second;
        floatBufferCache_.erase(iter);
        return tmp;
    }

    return context_.CreateBuffer<cl_float>(size, CL_MEM_READ_WRITE);
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
    floatBufferCache_.clear();
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

void CLWParallelPrimitives::ReclaimTempFloatBuffer(CLWBuffer<cl_float> buffer)
{
    auto iter = floatBufferCache_.find(buffer.GetElementCount());

    if (iter != floatBufferCache_.end())
    {
        return;
    }

    floatBufferCache_[buffer.GetElementCount()] = buffer;
}

CLWEvent CLWParallelPrimitives::Compact(unsigned int deviceIdx, CLWBuffer<cl_int> predicate, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, cl_int& newSize)
{
    /// Scan predicate array first to temp buffer

    size_t numElems = predicate.GetElementCount();

    CLWBuffer<cl_int> addresses = GetTempIntBuffer(numElems);

    ScanExclusiveAdd(deviceIdx, predicate, addresses).Wait();

    /// New size = scanned[last] + perdicate[last]
    /// TODO: remove this sync
    context_.ReadBuffer(deviceIdx, addresses, &newSize, numElems - 1, 1).Wait();

    cl_int lastPredicate = 0;
    context_.ReadBuffer(deviceIdx, predicate, &lastPredicate, numElems - 1, 1).Wait();
    newSize += lastPredicate;

    int NUM_BLOCKS =  (numElems + WG_SIZE - 1)/ WG_SIZE;

    CLWKernel compactKernel = program_.GetKernel("compact_int");

    compactKernel.SetArg(0, predicate);
    compactKernel.SetArg(1, addresses);
    compactKernel.SetArg(2, input);
    compactKernel.SetArg(3, (cl_uint)numElems);
    compactKernel.SetArg(4, output);

    /// TODO: unsafe as it is used in the kernel, may have problems on DMA devices
    ReclaimTempIntBuffer(addresses);

    return context_.Launch1D(deviceIdx, NUM_BLOCKS * WG_SIZE, WG_SIZE, compactKernel);
}


CLWEvent CLWParallelPrimitives::Copy(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output)
{
    int numElems = input.GetElementCount();
    int ELEMS_PER_WI = 4;
    int GROUP_BLOCK_SIZE = (WG_SIZE * ELEMS_PER_WI);
    int NUM_BLOCKS =  (numElems + GROUP_BLOCK_SIZE - 1)/ GROUP_BLOCK_SIZE;

    CLWKernel copyKernel = program_.GetKernel("copy");

    copyKernel.SetArg(0, input);
    copyKernel.SetArg(1, (cl_uint)input.GetElementCount());
    copyKernel.SetArg(2, output);

    return context_.Launch1D(0, NUM_BLOCKS * WG_SIZE, WG_SIZE, copyKernel);
}