//
//  CLWCommandQueue.h
//  CLW
//
//  Created by dmitryk on 02.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWCommandQueue__
#define __CLW__CLWCommandQueue__

#include <iostream>
#include <memory>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"

class CLWContext;
class CLWDevice;

class CLWCommandQueue : public ReferenceCounter<cl_command_queue, clRetainCommandQueue, clReleaseCommandQueue>
{
public:
    static CLWCommandQueue Create(CLWDevice device, CLWContext context);
    
    virtual          ~CLWCommandQueue();
    
private:
    CLWCommandQueue(cl_command_queue cmdQueue, CLWDevice device, CLWContext context);
};


#endif /* defined(__CLW__CLWCommandQueue__) */
