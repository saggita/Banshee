//
//  CLWContext.h
//  CLW
//
//  Created by dmitryk on 01.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWContext__
#define __CLW__CLWContext__

#include <iostream>
#include <memory>
#include <vector>
#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"
#include "ParameterHolder.h"
#include "CLWBuffer.h"
#include "CLWImage2D.h"
#include "CLWEvent.h"
#include "CLWDevice.h"

class CLWProgram;
class CLWCommandQueue;

class CLWContext : public ReferenceCounter<cl_context, clRetainContext, clReleaseContext>
{
public:

    static CLWContext Create(std::vector<CLWDevice> const&, cl_context_properties* props = nullptr);
    static CLWContext Create(CLWDevice device, cl_context_properties* props = nullptr);

    CLWContext(){}
    virtual                     ~CLWContext();

    unsigned int                GetDeviceCount() const;
    CLWDevice                   GetDevice(unsigned int idx) const;
    CLWProgram                  CreateProgram(std::vector<char> const& sourceCode) const;

    template <typename T> CLWBuffer<T>  CreateBuffer(size_t elementCount) const;
    template <typename T> CLWBuffer<T>  CreateBuffer(size_t elementCount, void* data) const;

    CLWImage2D                          CreateImage2DFromGLTexture(cl_GLint texture) const;

    template <typename T> CLWEvent  WriteBuffer(unsigned int idx, CLWBuffer<T> buffer, T* const hostBuffer, size_t elemCount) const;
    template <typename T> CLWEvent  FillBuffer(unsigned int idx, CLWBuffer<T> buffer, T const& val, size_t elemCount) const;
    template <typename T> CLWEvent  ReadBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* hostBuffer, size_t elemCount) const;
    template <typename T> CLWEvent  ReadBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* hostBuffer, size_t offset, size_t elemCount) const;
    template <typename T> CLWEvent  CopyBuffer(unsigned int idx,  CLWBuffer<T> source, CLWBuffer<T> dest, size_t srcOffset, size_t destOffset, size_t elemCount) const;
    template <typename T> CLWEvent  MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, T** mappedData) const;
    template <typename T> CLWEvent  MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData) const;
    template <typename T> CLWEvent  UnmapBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* mappedData) const;

    //variadic temporarily disabled due to VS13 requirement 
    //template <unsigned globalSize, unsigned localSize, typename ... Types> void Launch1D(unsigned int idx, cl_kernel kernel, Types ... args);
    CLWEvent Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel);
    CLWEvent Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, CLWEvent depEvent);
    CLWEvent Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, std::vector<CLWEvent> const& events);

    CLWEvent Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel);
    CLWEvent Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, CLWEvent depEvent);
    CLWEvent Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, std::vector<CLWEvent> const& events);

    void Finish(unsigned int idx) const;

    // GL interop 
    void AcquireGLObjects(unsigned int idx, std::vector<cl_mem> const& objects) const;
    void ReleaseGLObjects(unsigned int idx, std::vector<cl_mem> const& objects) const;

private:
    void InitCL();

    CLWContext(cl_context context, std::vector<CLWDevice> const&);
    CLWContext(cl_context context, std::vector<CLWDevice>&&);

    CLWContext(CLWDevice device);

    std::vector<CLWDevice>       devices_;
    std::vector<CLWCommandQueue> commandQueues_;
};

template <typename T> CLWBuffer<T> CLWContext::CreateBuffer(size_t elementCount) const
{
    return CLWBuffer<T>::Create(*this, elementCount);
}

template <typename T> CLWBuffer<T> CLWContext::CreateBuffer(size_t elementCount, void* data) const
{
    return CLWBuffer<T>::Create(*this, elementCount, data);
}

template <typename T> CLWEvent  CLWContext::WriteBuffer(unsigned int idx, CLWBuffer<T> buffer, T* const hostBuffer, size_t elemCount) const
{
    return buffer.WriteDeviceBuffer(commandQueues_[idx], hostBuffer, elemCount);
}

template <typename T> CLWEvent  CLWContext::FillBuffer(unsigned int idx, CLWBuffer<T> buffer, T const& val, size_t elemCount) const
{
    return buffer.FillDeviceBuffer(commandQueues_[idx], val, elemCount);
}

template <typename T> CLWEvent  CLWContext::ReadBuffer(unsigned int idx, CLWBuffer<T> buffer, T* hostBuffer, size_t elemCount) const
{
    return buffer.ReadDeviceBuffer(commandQueues_[idx], hostBuffer, elemCount);
}

template <typename T> CLWEvent  CLWContext::ReadBuffer(unsigned int idx, CLWBuffer<T> buffer, T* hostBuffer, size_t offset, size_t elemCount) const
{
    return buffer.ReadDeviceBuffer(commandQueues_[idx], hostBuffer, offset, elemCount);
}

//template <unsigned globalSize, unsigned localSize, typename ... Types> void CLWContext::Launch1D(unsigned int idx, cl_kernel kernel, Types ... args)
//{
//    std::vector<ParameterHolder> params{args...};
//    //std::vector<int>{args...};
//    for (unsigned int i = 0; i < params.size(); ++i)
//    {
//        params[i].SetArg(kernel, i);
//    }
//    
//    cl_int status = CL_SUCCESS;
//    size_t wgLocalSize = localSize;
//    size_t wgGlobalSize = globalSize;
//    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 1, nullptr, &wgGlobalSize, &wgLocalSize, 0, nullptr, nullptr);
//    
//    assert(status == CL_SUCCESS);
//    // TODO: add error handling
//}

template <typename T> CLWEvent  CLWContext::CopyBuffer(unsigned int idx, CLWBuffer<T> source, CLWBuffer<T> dest, size_t srcOffset, size_t destOffset, size_t elemCount) const
{
    cl_int status = CL_SUCCESS;
    cl_event event;

    clEnqueueCopyBuffer(commandQueues_[idx], source, dest, srcOffset * sizeof(T), destOffset* sizeof(T), elemCount * sizeof(T), 0, nullptr, &event); 
    assert(status == CL_SUCCESS);

    return CLWEvent::Create(event);
}

template <typename T> CLWEvent  CLWContext::MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, T** mappedData) const
{
    return buffer.MapDeviceBuffer(commandQueues_[idx], flags, mappedData);
}

template <typename T> CLWEvent  CLWContext::MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData) const
{
    return buffer.MapDeviceBuffer(commandQueues_[idx], flags, offset, elemCount, mappedData);
}

template <typename T> CLWEvent  CLWContext::UnmapBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* mappedData) const
{
    return buffer.UnmapDeviceBuffer(commandQueues_[idx], mappedData);
}




#endif /* defined(__CLW__CLWContext__) */
