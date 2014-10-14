//
//  CLWProgram.cpp
//  CLW
//
//  Created by dmitryk on 18.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWProgram.h"
#include "CLWContext.h"
#include "CLWDevice.h"
#include "CLWExcept.h"
#include "CLWKernel.h"

#include <functional>
#include <cassert>
#include <algorithm>

CLWProgram CLWProgram::CreateFromSource(std::vector<char> const& sourceCode, CLWContext context)
{
    cl_int status = CL_SUCCESS;
    size_t sourceSize = sourceCode.size();
    
    char const* tempPtr = &sourceCode[0];
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&tempPtr, &sourceSize, &status);
    
    ThrowIf<CLWOutOfHostMemory>(status == CL_OUT_OF_HOST_MEMORY, "Cannot create program: Out of host memory");
    ThrowIf<CLWInvalidValue>(status == CL_INVALID_VALUE, "Cannot create program: Invalid value");
    ThrowIf<CLWInvalidContext>(status == CL_INVALID_CONTEXT, "Cannot create program: Invalid value");
    
    std::vector<cl_device_id> deviceIds(context.GetDeviceCount());
    for(unsigned int i = 0; i < context.GetDeviceCount(); ++i)
    {
        deviceIds[i] = context.GetDevice(i);
    }

    status = clBuildProgram(program, context.GetDeviceCount(), &deviceIds[0], nullptr, nullptr, nullptr);

    if(status != CL_SUCCESS)
    {
        std::vector<char> buildLog;
        size_t logSize;
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);

        buildLog.resize(logSize);
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, logSize, &buildLog[0], nullptr);

        throw std::runtime_error(&buildLog[0]);
        assert(false);
    }
    
    CLWProgram prg(program);
    
    clReleaseProgram(program);

    //return  std::shared_ptr<CLWProgram>(new CLWProgram(sourceCode, context));
    return prg;
}

CLWProgram::CLWProgram(cl_program program)
: ReferenceCounter<cl_program, clRetainProgram, clReleaseProgram>(program)
{
    cl_int status = CL_SUCCESS;
    cl_uint numKernels;
    status = clCreateKernelsInProgram(*this, 0, nullptr, &numKernels);
    
    ThrowIf<CLWOutOfHostMemory>(status == CL_OUT_OF_HOST_MEMORY, "Cannot create kernels: Out of host memory");
    ThrowIf<CLWInvalidProgram>(status == CL_INVALID_PROGRAM, "Cannot create kernels: Invalid program");
    ThrowIf<CLWInvalidProgramExecutable>(status == CL_INVALID_PROPERTY, "Cannot kernels: Invalid program executable");
    ThrowIf<CLWInvalidValue>(status == CL_INVALID_VALUE, "Cannot create kernels: Invalid value");
    
    std::vector<cl_kernel> kernels(numKernels);
    status = clCreateKernelsInProgram(*this, numKernels, &kernels[0], nullptr);
    
    ThrowIf<CLWOutOfHostMemory>(status == CL_OUT_OF_HOST_MEMORY, "Cannot create kernels: Out of host memory");
    ThrowIf<CLWInvalidProgram>(status == CL_INVALID_PROGRAM, "Cannot create kernels: Invalid program");
    ThrowIf<CLWInvalidProgramExecutable>(status == CL_INVALID_PROPERTY, "Cannot kernels: Invalid program executable");
    ThrowIf<CLWInvalidValue>(status == CL_INVALID_VALUE, "Cannot create kernels: Invalid value");
    
    std::for_each(kernels.begin(), kernels.end(), [this](cl_kernel k)
                  {
                      size_t size = 0;
                      cl_int res;
                      
                      res = clGetKernelInfo(k, CL_KERNEL_FUNCTION_NAME, 0, nullptr, &size);
                      assert(res == CL_SUCCESS);
                      
                      std::vector<char> temp(size);
                      res = clGetKernelInfo(k, CL_KERNEL_FUNCTION_NAME, size, &temp[0], nullptr);
                      assert(res == CL_SUCCESS);
                      
                      std::string funcName(temp.begin(), temp.end()-1);
                      kernels_[funcName] = CLWKernel::Create(k);
                  });
}

CLWProgram::~CLWProgram()
{
}

unsigned int CLWProgram::GetKernelCount() const
{
    return static_cast<unsigned int>(kernels_.size());
}

CLWKernel CLWProgram::GetKernel(std::string const& funcName) const
{
    auto iter = kernels_.find(funcName);
    
    ThrowIf<CLWInvalidKernelName>(iter == kernels_.end(), "No such kernel in program");
    
    return iter->second;
}