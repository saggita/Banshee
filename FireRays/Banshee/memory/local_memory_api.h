#ifndef LOCALMEMORYAPI_H
#define LOCALMEMORYAPI_H

#include "memory_api.h"

class LocalMemoryApi : public MemoryApi
{
public:
    // Default constructor
    LocalMemoryApi(){}
    // Destructor
    ~LocalMemoryApi(){}
    
    // Get an id of a memory location managed by this instance of an API
    LocationId GetLocationId() const;
    // Api type
    ApiType    GetApiType() const;
    
protected:
    // Allocate memory area of a specified size
    RawMemoryArea* AllocateRaw(size_t size, void const* init_data = nullptr);
    // Copy data from local memory buffer into memory area
    void CopyRaw(RawMemoryArea& dst, void const* src, size_t offset, size_t size);
    // Copy data from memory area to local buffer
    void CopyRaw(void* dst, RawMemoryArea const& src, size_t offset, size_t size);
    // Set memory area data to local value
    void SetRaw(RawMemoryArea& dst, void const* pattern, size_t pattern_size, size_t offset, size_t repeat_count) ;
};

#endif // LOCALMEMORYAPI_H
