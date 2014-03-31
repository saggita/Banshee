//
//  CLWImage2D.h
//  CLW
//
//  Created by dmitryk on 19.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWImage2D__
#define __CLW__CLWImage2D__

#include <iostream>
#include <memory>
#include <vector>
#include <cassert>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#else
#include <CL/cl.h>
#include <CL/cl_gl.h>
#endif

#include "ReferenceCounter.h"
#include "ParameterHolder.h"
#include "CLWCommandQueue.h"
#include "CLWEvent.h"


class CLWImage2D : public ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>
{
public:
    static CLWImage2D Create(cl_context context, cl_image_format const* imgFormat, size_t width, size_t height, size_t rowPitch);
    static CLWImage2D CreateFromGLTexture(cl_context context, cl_GLint texture);

    CLWImage2D(){}
    virtual ~CLWImage2D();

    operator ParameterHolder() const
    {
        return ParameterHolder((cl_mem)*this);
    }
    
private:

    CLWImage2D(cl_mem image);
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
