//
//  CLWProgram.h
//  CLW
//
//  Created by dmitryk on 18.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//
#ifndef __CLW__CLWProgram__
#define __CLW__CLWProgram__

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "CLWKernel.h"
#include "ReferenceCounter.h"

class CLWContext;

class CLWProgram : public ReferenceCounter<cl_program, clRetainProgram, clReleaseProgram>
{
public:
    static CLWProgram CreateFromSource(std::vector<char> const& sourceCode, CLWContext context);
    
    CLWProgram() {}
    virtual      ~CLWProgram();

    unsigned int GetKernelCount() const;
    CLWKernel    GetKernel(std::string const& funcName) const;
    
private:
    CLWProgram(cl_program program);
    std::map<std::string, CLWKernel> kernels_;
};

#endif /* defined(__CLW__CLWProgram__) */
