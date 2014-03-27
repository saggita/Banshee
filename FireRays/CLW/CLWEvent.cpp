//
//  CLWEvent.cpp
//  CLW
//
//  Created by dmitryk on 26.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWEvent.h"

#include <cassert>

CLWEvent CLWEvent::Create(cl_event event)
{
    CLWEvent e(event);
    clReleaseEvent(event);
    return e;
}

CLWEvent::CLWEvent(cl_event event)
: ReferenceCounter<cl_event, clRetainEvent, clReleaseEvent>(event)
{
}

void CLWEvent::Wait()
{
    cl_event event = *this;
    cl_int status = clWaitForEvents(1, &event);

    assert(status == CL_SUCCESS);
}

CLWEvent::~CLWEvent()
{
}