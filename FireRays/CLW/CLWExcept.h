//
//  CLWExcept.h
//  CLW
//
//  Created by dmitryk on 30.11.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#ifndef CLW_CLWExcept_h
#define CLW_CLWExcept_h

#include <string>

class CLWExcept : public std::runtime_error
{
public:
    CLWExcept(std::string const& message) : std::runtime_error(message) {} 
};

class CLWInvalidPlatform : public CLWExcept
{
public:
    CLWInvalidPlatform(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidContext : public CLWExcept
{
public:
    CLWInvalidContext(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidDevice : public CLWExcept
{
public:
    CLWInvalidDevice(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidValue : public CLWExcept
{
public:
    CLWInvalidValue(std::string const& message) : CLWExcept(message) {}
};

class CLWDeviceNotAvailable : public CLWExcept
{
public:
    CLWDeviceNotAvailable(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidQueueProperties : public CLWExcept
{
public:
    CLWInvalidQueueProperties(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidProgram : public CLWExcept
{
public:
    CLWInvalidProgram(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidProgramExecutable : public CLWExcept
{
public:
    CLWInvalidProgramExecutable(std::string const& message) : CLWExcept(message) {}
};

class CLWBuildProgramFailure : public CLWExcept
{
public:
    CLWBuildProgramFailure(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidKernelName : public CLWExcept
{
public:
    CLWInvalidKernelName(std::string const& message) : CLWExcept(message) {}
};

class CLWOutOfHostMemory : public CLWExcept
{
public:
    CLWOutOfHostMemory(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidBufferSize : public CLWExcept
{
public:
    CLWInvalidBufferSize(std::string const& message) : CLWExcept(message) {}
};

class CLWInvalidHostPtr : public CLWExcept
{
public:
    CLWInvalidHostPtr(std::string const& message) : CLWExcept(message) {}
};

class CLWMemObjectAllocationFailure : public CLWExcept
{
public:
    CLWMemObjectAllocationFailure(std::string const& message) : CLWExcept(message) {}
};

template <typename Exception> inline void ThrowIf(bool condition, std::string const& message)
{
    if (condition)
        throw Exception(message);
}

#endif
