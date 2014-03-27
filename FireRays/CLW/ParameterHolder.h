//
//  ParameterHolder.h
//  CLW
//
//  Created by dmitryk on 23.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__ParameterHolder__
#define __CLW__ParameterHolder__

#include <iostream>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif


struct SharedMemory
{
    SharedMemory(cl_uint size) : size_(size) {}
    
    cl_uint size_;
};

class ParameterHolder
{
public:
    enum Type
    {
        kMem,
        kInt,
        kUInt,
        kFloat,
        kFloat2,
        kFloat4,
        kDouble,
        kShmem
    };
    
    ParameterHolder(cl_mem    mem) : mem_(mem) { type_ = kMem; }
    ParameterHolder(cl_int    intValue) : intValue_(intValue) { type_ = kInt; }
    ParameterHolder(cl_uint   uintValue) : uintValue_(uintValue) { type_ = kUInt; }
    ParameterHolder(cl_float  floatValue) : floatValue_(floatValue) { type_ = kFloat; }
    ParameterHolder(cl_float2  floatValue) : floatValue2_(floatValue) { type_ = kFloat2; }
    ParameterHolder(cl_float4  floatValue) : floatValue4_(floatValue) { type_ = kFloat4; }
    ParameterHolder(cl_double doubleValue) : doubleValue_(doubleValue) { type_ = kDouble; }
    ParameterHolder(SharedMemory shMem) : uintValue_(shMem.size_) { type_ = kShmem; }
    
    void SetArg(cl_kernel kernel, unsigned int idx);
    
private:
    
    Type type_;
    
    union {
        cl_mem    mem_;
        cl_int    intValue_;
        cl_uint   uintValue_;
        cl_float  floatValue_;
        cl_float2  floatValue2_;
        cl_float4  floatValue4_;
        cl_double doubleValue_;
    };
};

#endif /* defined(__CLW__ParameterHolder__) */
