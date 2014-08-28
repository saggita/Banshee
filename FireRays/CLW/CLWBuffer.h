//
//  CLWBuffer.h
//  CLW
//
//  Created by dmitryk on 19.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWBuffer__
#define __CLW__CLWBuffer__

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
#include "CLWExcept.h"


template <typename T> class CLWBuffer : public ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>
{
public:
    static CLWBuffer<T> Create(cl_context context, cl_mem_flags flags, size_t elementCount);
    static CLWBuffer<T> Create(cl_context context, cl_mem_flags flags, size_t elementCount, void* data);

    CLWBuffer(){}
    virtual ~CLWBuffer();
    
    size_t GetElementCount() const { return elementCount_; }
    
    operator ParameterHolder() const
    {
        return ParameterHolder((cl_mem)*this);
    }
    
private:
    CLWEvent WriteDeviceBuffer(CLWCommandQueue cmdQueue, T const* hostBuffer, size_t elemCount);
    CLWEvent FillDeviceBuffer(CLWCommandQueue cmdQueue, T const& val, size_t elemCount);
    CLWEvent ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t elemCount);
    CLWEvent ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t offset, size_t elemCount);
    CLWEvent MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, T** mappedData);
    CLWEvent MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData);
    CLWEvent UnmapDeviceBuffer(CLWCommandQueue cmdQueue, T* mappedData);

    CLWBuffer(cl_mem buffer, size_t elementCount);

    size_t elementCount_;
    
    friend class CLWContext;
};

template <typename T> CLWBuffer<T> CLWBuffer<T>::Create(cl_context context, cl_mem_flags flags, size_t elementCount)
{
    cl_int status = CL_SUCCESS;
    cl_mem deviceBuffer = clCreateBuffer(context, flags, elementCount * sizeof(T), nullptr, &status);

    ThrowIf<CLWInvalidContext>(status == CL_INVALID_CONTEXT, "CreateBuffer: invalid context passed");
    ThrowIf<CLWInvalidValue>(status == CL_INVALID_VALUE, "CreateBuffer: invalid combination of flags passed");
    ThrowIf<CLWInvalidBufferSize>(status == CL_INVALID_BUFFER_SIZE, "CreateBuffer: buffer size exceeds allocation limit for the device");
    ThrowIf<CLWInvalidHostPtr>(status == CL_INVALID_HOST_PTR, "CreateBuffer: invalid host ptr value passed");
    ThrowIf<CLWMemObjectAllocationFailure>(status == CL_MEM_OBJECT_ALLOCATION_FAILURE, "CreateBuffer: allocation failed");
    ThrowIf<CLWOutOfHostMemory>(status == CL_OUT_OF_HOST_MEMORY, "CreateBuffer: out of host memory");

    CLWBuffer<T> buffer(deviceBuffer, elementCount);

    clReleaseMemObject(deviceBuffer);

    return buffer;
}

template <typename T> CLWBuffer<T> CLWBuffer<T>::Create(cl_context context, cl_mem_flags flags, size_t elementCount, void* data)
{
    cl_int status = CL_SUCCESS;
    cl_mem deviceBuffer = clCreateBuffer(context, flags, elementCount * sizeof(T), data, &status);

    ThrowIf<CLWInvalidContext>(status == CL_INVALID_CONTEXT, "CreateBuffer: invalid context passed");
    ThrowIf<CLWInvalidValue>(status == CL_INVALID_VALUE, "CreateBuffer: invalid combination of flags passed");
    ThrowIf<CLWInvalidBufferSize>(status == CL_INVALID_BUFFER_SIZE, "CreateBuffer: buffer size exceeds allocation limit for the device");
    ThrowIf<CLWInvalidHostPtr>(status == CL_INVALID_HOST_PTR, "CreateBuffer: invalid host ptr value passed");
    ThrowIf<CLWMemObjectAllocationFailure>(status == CL_MEM_OBJECT_ALLOCATION_FAILURE, "CreateBuffer: allocation failed");
    ThrowIf<CLWOutOfHostMemory>(status == CL_OUT_OF_HOST_MEMORY, "CreateBuffer: out of host memory");

    CLWBuffer<T> buffer(deviceBuffer, elementCount);

    clReleaseMemObject(deviceBuffer);

    return buffer;
}

template <typename T> CLWBuffer<T>::CLWBuffer(cl_mem buffer, size_t elementCount)
: ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>(buffer)
, elementCount_(elementCount)
{
   
}

template <typename T> CLWBuffer<T>::~CLWBuffer()
{
}

template <typename T> CLWEvent CLWBuffer<T>::WriteDeviceBuffer(CLWCommandQueue cmdQueue, T const* hostBuffer, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueWriteBuffer(cmdQueue, *this, false, 0, sizeof(T)*elemCount, hostBuffer, 0, nullptr, &event);
    
    assert(status == CL_SUCCESS);
    // TODO: handle errors
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueReadBuffer(cmdQueue, *this, false, 0, sizeof(T)*elemCount, hostBuffer, 0, nullptr, &event);
    
    assert(status == CL_SUCCESS);
    // TODO: handle errors
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t offset, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueReadBuffer(cmdQueue, *this, false, sizeof(T)*offset, sizeof(T)*elemCount, hostBuffer, 0, nullptr, &event);
    
    assert(status == CL_SUCCESS);
    // TODO: handle errors
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, T** mappedData)
{
    assert(mappedData);

    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    T* data = (T*)clEnqueueMapBuffer(cmdQueue, *this, false, flags, 0, sizeof(T)*elementCount_, 0, nullptr, &event, &status);

    assert(status == CL_SUCCESS);

    *mappedData = data;

    // TODO: handle errors
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData)
{
    assert(mappedData);
    
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    
    T* data = (T*)clEnqueueMapBuffer(cmdQueue, *this, false, flags, sizeof(T) * offset, sizeof(T)*elemCount, 0, nullptr, &event, &status);
    
    assert(status == CL_SUCCESS);
    
    *mappedData = data;
    
    // TODO: handle errors
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::UnmapDeviceBuffer(CLWCommandQueue cmdQueue, T* mappedData)
{
    assert(mappedData);

    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    status = clEnqueueUnmapMemObject(cmdQueue, *this,  mappedData, 0, nullptr, &event);

    assert(status == CL_SUCCESS);
    // TODO: handle errors
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::FillDeviceBuffer(CLWCommandQueue cmdQueue, T const& val, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueFillBuffer(cmdQueue, *this, &val, sizeof(T), 0, sizeof(T)*elemCount, 0, nullptr, &event);
    
    assert(status == CL_SUCCESS);
    // TODO: handle errors
    return CLWEvent::Create(event);
}


#endif /* defined(__CLW__CLWBuffer__) */
