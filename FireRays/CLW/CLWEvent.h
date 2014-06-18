//
//  CLWEvent.h
//  CLW
//
//  Created by dmitryk on 18.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//
#ifndef __CLW__CLWEvent__
#define __CLW__CLWEvent__

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

#include "ReferenceCounter.h"

class CLWEvent : public ReferenceCounter<cl_event, clRetainEvent, clReleaseEvent>
{
public:
    static CLWEvent Create(cl_event);
    CLWEvent(){}
    virtual      ~CLWEvent();

    void Wait();
    float GetDuration() const;

private:
    CLWEvent(cl_event program);
};

#endif /* defined(__CLW__CLWEvent__) */