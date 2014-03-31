//
//  CLWTexture.h
//  CLW
//
//  Created by dmitryk on 19.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWImage__
#define __CLW__CLWImage__

#include <iostream>
#include <memory>
#include <vector>
#include <cassert>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"
#include "ParameterHolder.h"
#include "CLWCommandQueue.h"
#include "CLWEvent.h"


class CLWImage : public ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>
{
public:
    static CLWImage Create(size_t elementCount, cl_context context);
    CLWImage(){}
    virtual ~CLWImage();

    operator ParameterHolder() const
    {
        return ParameterHolder((cl_mem)*this);
    }
    
private:
    //CLWEvent WriteDeviceBuffer(CLWCommandQueue cmdQueue, T const* hostBuffer, size_t elemCount);
    //CLWEvent ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t elemCount);
    //CLWEvent ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t offset, size_t elemCount);
    
    CLWImage(cl_mem image);

    friend class CLWContext;
};

//template <typename T> CLWBuffer<T> CLWBuffer<T>::Create(size_t elementCount, cl_context context)
//{
//    cl_int status = CL_SUCCESS;
//    cl_mem deviceBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, elementCount * sizeof(T), nullptr, &status);
//    
//    assert(status == CL_SUCCESS);
//    CLWBuffer<T> buffer(deviceBuffer, elementCount);
//    
//    clReleaseMemObject(deviceBuffer);
//    
//    return buffer;
//}
//
//template <typename T> CLWBuffer<T>::CLWBuffer(cl_mem buffer, size_t elementCount)
//: ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>(buffer)
//, elementCount_(elementCount)
//{
//   
//}
//
//template <typename T> CLWBuffer<T>::~CLWBuffer()
//{
//}



#endif /* defined(__CLW__CLWBuffer__) */
