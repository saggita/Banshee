#ifndef LOCALMEMORYAREA_H
#define LOCALMEMORYAREA_H

#include "memory_area.h"

class LocalMemoryArea : public RawMemoryArea
{
public:
    LocalMemoryArea(MemoryApi& api, size_t size);
    
    // Destructor
    ~LocalMemoryArea();
    
    // Get local pointer (only for LOCAL buffers)
    void*       raw_ptr() { return buffer_; }
    void const* raw_ptr() const { return buffer_; }
    
    // Get memory area size
    size_t      size() const { return size_; }
    
    // Get API that produced this area
    MemoryApi&  GetApi() const { return api_; };
    
private:
    
    MemoryApi&          api_;
    unsigned char*      buffer_;
    size_t              size_;
};

inline LocalMemoryArea::LocalMemoryArea(MemoryApi& api, size_t size) :
    api_(api)
    , buffer_(new unsigned char[size])
    , size_(size)
{
}


inline LocalMemoryArea::~LocalMemoryArea()
{
    delete [] buffer_;
}



#endif // LOCALMEMORYAREA_H
