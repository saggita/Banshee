//
//  CLWPlatform.h
//  CLW
//
//  Created by dmitryk on 30.11.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef __CLW__CLWPlatform_h__
#define __CLW__CLWPlatform_h__

#include <vector>
#include <string>
#include <memory>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"

class CLWDevice;

// Represents OpenCL platform
// Create CLWPlatfrom with CLWPlatform::Create function
class CLWPlatform : public ReferenceCounter<cl_platform_id, nullptr, nullptr>
{
public:
    // Create platform based on platform_id passed
    // CLWInvalidID is thrown if the id is not a valid OpenCL platform ID
    static CLWPlatform Create(cl_platform_id id);
    static void CreateAllPlatforms(std::vector<CLWPlatform>& platforms);
    
    virtual ~CLWPlatform();
    
    // Platform info
    std::string GetName() const;
    std::string GetProfile() const;
    std::string GetVersion() const;
    std::string GetVendor()  const;
    std::string GetExtensions() const;
    
    // Get number of devices
    unsigned int                GetDeviceCount();
    // Get idx-th device
    CLWDevice  GetDevice(unsigned int idx);
    
private:
    CLWPlatform(cl_platform_id id);
    void GetPlatformInfoParameter(cl_platform_id id, cl_platform_info param, std::string& result);
    void InitDeviceList();
    
    //CLWPlatform(CLWPlatform const&);
    //CLWPlatform& operator = (CLWPlatform const&);

    std::string name_;
    std::string profile_;
    std::string version_;
    std::string vendor_;
    std::string extensions_;

    std::vector<CLWDevice> devices_;
};





#endif
