//
//  CLWCommandQueue.cpp
//  CLW
//
//  Created by dmitryk on 02.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWCommandQueue.h"
#include "CLWContext.h"
#include "CLWDevice.h"
#include "CLWExcept.h"

CLWCommandQueue CLWCommandQueue::Create(CLWDevice device, CLWContext context)
{
    cl_int status = CL_SUCCESS;
    
    cl_command_queue commandQueue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
    
    ThrowIf<CLWOutOfHostMemory>(status == CL_OUT_OF_HOST_MEMORY, "Cannot create command queue: Out of host memory");
    ThrowIf<CLWInvalidDevice>(status == CL_INVALID_DEVICE, "Cannot create command queue: Invalid device");
    ThrowIf<CLWInvalidQueueProperties>(status == CL_INVALID_QUEUE_PROPERTIES, "Cannot create context: Invalid queue properties");
    ThrowIf<CLWInvalidValue>(status == CL_INVALID_VALUE, "Cannot create command queue: Invalid value");
    ThrowIf<CLWInvalidContext>(status == CL_INVALID_CONTEXT, "Cannot create command queue: Invalid value");
    
    CLWCommandQueue cmdQueue(commandQueue, device, context);
    
    clReleaseCommandQueue(commandQueue);
    
    return cmdQueue;
}

CLWCommandQueue::CLWCommandQueue(cl_command_queue cmdQueue, CLWDevice device, CLWContext context)
: ReferenceCounter<cl_command_queue, clRetainCommandQueue, clReleaseCommandQueue>(cmdQueue)
{
}

CLWCommandQueue::~CLWCommandQueue()
{
}

