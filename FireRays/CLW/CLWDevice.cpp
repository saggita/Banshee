//
//  CLWDevice.cpp
//  CLW
//
//  Created by dmitryk on 01.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWDevice.h"

CLWDevice::CLWDevice(cl_device_id id) : ReferenceCounter<cl_device_id, clRetainDevice, clReleaseDevice>(id)
{
    GetDeviceInfoParameter(*this, CL_DEVICE_NAME, name_);
    GetDeviceInfoParameter(*this, CL_DEVICE_EXTENSIONS, extensions_);
    GetDeviceInfoParameter(*this, CL_DEVICE_VENDOR, vendor_);
    GetDeviceInfoParameter(*this, CL_DEVICE_VERSION, version_);
    GetDeviceInfoParameter(*this, CL_DEVICE_PROFILE, profile_);
    GetDeviceInfoParameter(*this, CL_DEVICE_TYPE, type_);
    
    GetDeviceInfoParameter(*this, CL_DEVICE_MAX_WORK_GROUP_SIZE, maxWorkGroupSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_GLOBAL_MEM_SIZE, globalMemSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_LOCAL_MEM_SIZE, localMemSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_LOCAL_MEM_TYPE, localMemType_);
}

CLWDevice::~CLWDevice()
{
}

template <> void CLWDevice::GetDeviceInfoParameter<std::string>(cl_device_id id, cl_device_info param, std::string& value)
{
    size_t length = 0;
    
    cl_int status = clGetDeviceInfo(id, param, 0, nullptr, &length);
    assert(status == CL_SUCCESS);
    
    std::vector<char> buffer(length);
    clGetDeviceInfo(id, param, length, &buffer[0], nullptr);

    value = &buffer[0];
}

std::string  CLWDevice::GetName() const
{
    return name_;
}

std::string  CLWDevice::GetExtensions() const
{
    return extensions_;
}

std::string  CLWDevice::GetVersion() const
{
    return version_;
}

std::string  CLWDevice::GetProfile() const
{
    return profile_;
}

std::string  CLWDevice::GetVendor() const
{
    return vendor_;
}

cl_device_type CLWDevice::GetType() const
{
    return type_;
}

// Device info
cl_ulong CLWDevice::GetLocalMemSize() const
{
    return localMemSize_;
}

cl_ulong CLWDevice::GetGlobalMemSize() const
{
    return globalMemSize_;
}

size_t   CLWDevice::GetMaxWorkGroupSize() const
{
    return maxWorkGroupSize_;
}

cl_device_id CLWDevice::GetID() const
{
    return *this;
}


