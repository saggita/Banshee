//
//  CLWPlatform.cpp
//  CLW
//
//  Created by dmitryk on 01.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWPlatform.h"
#include "CLWDevice.h"
#include "CLWExcept.h"

#include <cassert>
#include <algorithm>

// Create platform based on platform_id passed
// CLWInvalidID is thrown if the id is not a valid OpenCL platform ID
CLWPlatform CLWPlatform::Create(cl_platform_id id)
{
    return CLWPlatform(id);
}

void CLWPlatform::CreateAllPlatforms(std::vector<CLWPlatform>& platforms)
{
    cl_int status = CL_SUCCESS;
    cl_uint numPlatforms;
    status = clGetPlatformIDs(0, nullptr, &numPlatforms);
    assert(status == CL_SUCCESS);
    // TODO: handle errors
    
    std::vector<cl_platform_id> platformIds(numPlatforms);
    clGetPlatformIDs(numPlatforms, &platformIds[0], nullptr);
    
    platforms.clear();
    std::transform(platformIds.cbegin(), platformIds.cend(), std::back_inserter(platforms), CLWPlatform::Create);
}

CLWPlatform::~CLWPlatform()
{
}

void CLWPlatform::GetPlatformInfoParameter(cl_platform_id id, cl_platform_info param, std::string& result)
{
    size_t length = 0;
    
    cl_int status = clGetPlatformInfo(id, param, 0, nullptr, &length);
    assert(status == CL_SUCCESS);
    
    ThrowIf<CLWInvalidPlatform>(CL_INVALID_PLATFORM == status, "ID passed to CLWPlatform::Create is not a valid OpenCL platform ID");
    
    std::vector<char> buffer(length);
    clGetPlatformInfo(id, param, length, &buffer[0], nullptr);

    //result.assign(buffer.begin(), buffer.end() - 1);
    result = &buffer[0];
}

// Would be good to make the constructor private to
// prohibit the construction on the stack, but
// need a trick to use make_shared<> in this case
CLWPlatform::CLWPlatform(cl_platform_id id)
: ReferenceCounter<cl_platform_id, nullptr, nullptr>(id)
{
    GetPlatformInfoParameter(*this, CL_PLATFORM_NAME, name_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_PROFILE, profile_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_VENDOR, vendor_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_VERSION, version_);
}

// Platform info
std::string CLWPlatform::GetName() const
{
    return name_;
}

std::string CLWPlatform::GetProfile() const
{
    return profile_;
}

std::string CLWPlatform::GetVersion() const
{
    return version_;
}

std::string CLWPlatform::GetVendor()  const
{
    return vendor_;
}

std::string CLWPlatform::GetExtensions() const
{
    return extensions_;
}

// Get number of devices
unsigned int                CLWPlatform::GetDeviceCount()
{
    if (devices_.size() == 0)
    {
        InitDeviceList();
    }
    
    return (unsigned int)devices_.size();
}

// Get idx-th device
CLWDevice CLWPlatform::GetDevice(unsigned int idx)
{
    if (devices_.size() == 0)
    {
        InitDeviceList();
    }
    
    return devices_[idx];
}

void CLWPlatform::InitDeviceList()
{
    cl_uint numDevices = 0;
    cl_int status = clGetDeviceIDs(*this, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    assert(status == CL_SUCCESS);
    
    std::vector<cl_device_id> deviceIds(numDevices);
    status = clGetDeviceIDs(*this, CL_DEVICE_TYPE_ALL, numDevices, &deviceIds[0], nullptr);
    assert(status == CL_SUCCESS);
    
    for (cl_uint i=0; i < numDevices; ++i)
    {
        devices_.push_back(CLWDevice(deviceIds[i]));
    }

}
