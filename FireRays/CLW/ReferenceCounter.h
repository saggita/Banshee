//
//  ReferenceCounter.h
//  CLW
//
//  Created by dmitryk on 22.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__ReferenceCounter__
#define __CLW__ReferenceCounter__

#include <iostream>

#ifdef __APPLE__
#define STDCALL
#include <OpenCL/OpenCL.h>
#elif WIN32
#define STDCALL __stdcall
#include <CL/cl.h>
#else
#define STDCALL
#include <CL/cl.h>
#endif

template <typename T, cl_int(STDCALL *Retain)(T), cl_int(STDCALL *Release)(T)>
class ReferenceCounter
{
public:
    typedef ReferenceCounter<T, Retain, Release> SelfType;
    ReferenceCounter() : object_(nullptr) {}
    explicit ReferenceCounter(T object) : object_(object)
    {
        RetainObject();
    }
    
    ~ReferenceCounter()
    {
        ReleaseObject();
    }
    
    ReferenceCounter(SelfType const& rhs)
    {
        
        object_ = rhs.object_;
        
        RetainObject();
    }
    
    SelfType& operator = (SelfType const& rhs)
    {
        if (&rhs != this)
        {
            ReleaseObject();
            
            object_ = rhs.object_;
            
            RetainObject();
        }
        
        return *this;
    }
    
    operator T() const
    {
        return object_;
    }
    
private:
    void RetainObject()  { if (object_) Retain(object_); }
    void ReleaseObject() { if (object_) Release(object_); }
    
    T object_;
};

template <>
class ReferenceCounter<cl_platform_id, nullptr, nullptr>
{
public:
    typedef ReferenceCounter<cl_platform_id, nullptr, nullptr> SelfType;
    ReferenceCounter() : object_(nullptr) {}
    explicit ReferenceCounter(cl_platform_id object) : object_(object)
    {
    }
    
    ~ReferenceCounter()
    {
    }
    
    ReferenceCounter(SelfType const& rhs)
    {
        object_ = rhs.object_;
    }
    
    SelfType& operator = (SelfType const& rhs)
    {
        if (&rhs != this)
        {
            object_ = rhs.object_;
        }
        
        return *this;
    }
    
    operator cl_platform_id() const
    {
        return object_;
    }
    
private:
    
    cl_platform_id object_;
};


#endif /* defined(__CLW__ReferenceCounter__) */
