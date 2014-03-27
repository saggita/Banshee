//
//  CLWDevice.h
//  CLW
//
//  Created by dmitryk on 30.11.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWDevice_h__
#define __CLW__CLWDevice_h__

#include <memory>
#include <cassert>
#include <vector>
#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"

class CLWPlatform;

class CLWDevice : public ReferenceCounter<cl_device_id, clRetainDevice, clReleaseDevice>
{
public:
    virtual ~CLWDevice();

    // Device info
    cl_ulong GetLocalMemSize() const;
    cl_ulong GetGlobalMemSize() const;
    size_t   GetMaxWorkGroupSize() const;
    cl_device_type GetType() const;
    cl_device_id GetID() const;

    // ... GetExecutionCapabilties() const;
    std::string  GetName() const;
    std::string  GetVendor() const;
    std::string  GetVersion() const;
    std::string  GetProfile() const;
    std::string  GetExtensions() const;

    // unsigned int GetGlobalMemCacheSize() const;
    // ...
    // ...

private:
    template <typename T> void GetDeviceInfoParameter(cl_device_id id, cl_device_info param, T& value);

    //CLWDevice(CLWDevice const&);
    //CLWDevice& operator = (CLWDevice const&);
    CLWDevice(cl_device_id id);
    
    std::string              name_;
    std::string              vendor_;
    std::string              version_;
    std::string              profile_;
    std::string              extensions_;
    cl_device_type           type_;
    
    cl_ulong                 localMemSize_;
    cl_ulong                 globalMemSize_;
    cl_device_local_mem_type localMemType_;
    size_t                   maxWorkGroupSize_;
    
    friend class CLWPlatform;
};

template <typename T> inline void CLWDevice::GetDeviceInfoParameter(cl_device_id id, cl_device_info param, T& value)
{
    cl_int status = clGetDeviceInfo(id, param, sizeof(T), &value, nullptr);
    assert(CL_SUCCESS == status);
}

template <> void CLWDevice::GetDeviceInfoParameter<std::string>(cl_device_id id, cl_device_info param, std::string& value);


#endif
