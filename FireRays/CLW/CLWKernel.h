//
//  CLWKernel.h
//  CLW
//
//  Created by dmitryk on 26.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWKernel__
#define __CLW__CLWKernel__

#include <iostream>

#include "ReferenceCounter.h"

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

class ParameterHolder;

class CLWKernel : public ReferenceCounter<cl_kernel, clRetainKernel, clReleaseKernel>
{
public:
    static CLWKernel Create(cl_kernel kernel);
    /// to use in std::map
    CLWKernel(){}
    virtual ~CLWKernel(){}

    virtual void SetArg(unsigned int idx, ParameterHolder param);

private:
    CLWKernel(cl_kernel kernel);
};

#endif /* defined(__CLW__CLWKernel__) */
