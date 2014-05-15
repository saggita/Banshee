//
//  CLWParallelPrimitives.h
//  CLW
//
//  Created by dmitryk on 11.03.14.
//  Copyright (c) 2014 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWParallelPrimitives__
#define __CLW__CLWParallelPrimitives__

#include "CLWContext.h"
#include "CLWProgram.h"
#include "CLWEvent.h"
#include "CLWBuffer.h"

class CLWParallelPrimitives
{
public:
    // Create primitive instances for the context
    CLWParallelPrimitives(CLWContext context);
    CLWParallelPrimitives(){}
    ~CLWParallelPrimitives();

    ///  TODO: Make these templates at some point
    CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);
    //CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_uint> input, CLWBuffer<cl_uint> output);
    //CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_short> input, CLWBuffer<cl_short> output);
    //CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_ushort> input, CLWBuffer<cl_ushort> output);

    CLWEvent SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys,
                                               CLWBuffer<cl_int> inputValues, CLWBuffer<cl_int> outputValues);

    void ReclaimDeviceMemory();

protected:
    CLWEvent ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);
    CLWEvent ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);
    CLWEvent ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);

    CLWBuffer<cl_int> GetTempIntBuffer(size_t size);
    void              ReclaimTempIntBuffer(CLWBuffer<cl_int> buffer);

private:
    CLWContext context_;
    CLWProgram program_;

    std::map<size_t, CLWBuffer<cl_int> > intBufferCache_;
};


#endif /* defined(__CLW__CLWParallelPrimitives__) */
