//
//  CLWContext.cpp
//  CLW
//
//  Created by dmitryk on 01.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWContext.h"
#include "CLWCommandQueue.h"
#include "CLWDevice.h"
#include "CLWProgram.h"
#include "CLWExcept.h"

#include <algorithm>



CLWContext CLWContext::Create(std::vector<CLWDevice> const& devices, CreationFlags creationFlags)
{
    std::vector<cl_device_id> deviceIds;
    std::for_each(devices.cbegin(), devices.cend(),
                  [&deviceIds](CLWDevice const& device)
                  {
                      deviceIds.push_back(device);
                  });
    cl_int status = CL_SUCCESS;
    cl_context ctx = clCreateContext(nullptr, static_cast<cl_int>(deviceIds.size()), &deviceIds[0], nullptr, nullptr, &status);
    
    ThrowIf<CLWOutOfHostMemory>(status == CL_OUT_OF_HOST_MEMORY, "Cannot create context: Out of host memory");
    ThrowIf<CLWInvalidDevice>(status == CL_INVALID_DEVICE, "Cannot create context: Invalid device");
    ThrowIf<CLWInvalidPlatform>(status == CL_INVALID_PLATFORM, "Cannot create context: Invalid platform");
    ThrowIf<CLWInvalidValue>(status == CL_INVALID_VALUE, "Cannot create context: Invalid value");
    ThrowIf<CLWDeviceNotAvailable>(status == CL_DEVICE_NOT_AVAILABLE, "Cannot create context: Device not available");
    
    CLWContext context(ctx, devices, creationFlags);
    
    clReleaseContext(ctx);
    
    return context;
}

CLWEvent CLWContext::Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel)
{
    cl_int status = CL_SUCCESS;
    size_t wgLocalSize = localSize;
    size_t wgGlobalSize = globalSize;
    cl_event event = nullptr;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 1, nullptr, &wgGlobalSize, &wgLocalSize, 0, nullptr, &event);
    assert(status == CL_SUCCESS);

    return CLWEvent::Create(event);
}


CLWEvent CLWContext::Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, CLWEvent depEvent)
{
    cl_int status = CL_SUCCESS;
    size_t wgLocalSize = localSize;
    size_t wgGlobalSize = globalSize;
    cl_event event = nullptr;
    cl_event eventToWait = depEvent;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 1, nullptr, &wgGlobalSize, &wgLocalSize, 1, &eventToWait, &event);
    assert(status == CL_SUCCESS);

    return CLWEvent::Create(event);
}


CLWEvent CLWContext::Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, std::vector<CLWEvent> const& events)
{
    cl_int status = CL_SUCCESS;
    size_t wgLocalSize = localSize;
    size_t wgGlobalSize = globalSize;
    cl_event event = nullptr;
    std::vector<cl_event> eventsToWait(events.size());

    for (unsigned i = 0; i < events.size(); ++i)
        eventsToWait[i] = events[i];

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 1, nullptr, &wgGlobalSize, &wgLocalSize, (cl_uint)eventsToWait.size(), &eventsToWait[0], &event);
    assert(status == CL_SUCCESS);

    return CLWEvent::Create(event);
}

CLWContext CLWContext::Create(CLWDevice device, CreationFlags creationFlags)
{
    std::vector<CLWDevice> devices;
    devices.push_back(device);
    return CLWContext::Create(devices, creationFlags);
}

CLWContext::CLWContext(cl_context context, std::vector<CLWDevice> const& devices, CreationFlags creationFlags)
: ReferenceCounter<cl_context, clRetainContext, clReleaseContext>(context)
, devices_(devices)
{
    InitCL(creationFlags);
}

CLWContext::CLWContext(cl_context context, std::vector<CLWDevice>&& devices, CreationFlags creationFlags)
: ReferenceCounter<cl_context, clRetainContext, clReleaseContext>(context)
, devices_(devices)
{
    InitCL(creationFlags);
}

CLWContext::CLWContext(CLWDevice device, CreationFlags creationFlags)
{
    devices_.push_back(device);
    InitCL(creationFlags);
}

CLWContext::~CLWContext()
{
}

unsigned int CLWContext::GetDeviceCount() const
{
    return (unsigned int)devices_.size();
}

CLWDevice CLWContext::GetDevice(unsigned int idx) const
{
    return devices_[idx];
}

void CLWContext::InitCL(CreationFlags creationFlags)
{
    std::for_each(devices_.begin(), devices_.end(),
                  [this](CLWDevice const& device)
                  {
                      commandQueues_.push_back(CLWCommandQueue::Create(device, *this));
                  });
}

CLWProgram CLWContext::CreateProgram(std::vector<char> const& sourceCode) const
{
    return CLWProgram::CreateFromSource(sourceCode, *this);
}


