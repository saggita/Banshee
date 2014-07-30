#ifndef MEMORYAPI_H
#define MEMORYAPI_H

#include <memory>

class RawMemoryArea;
template <typename T> class MemoryArea;

typedef int LocationId;

///< MemoryApi is a key abstraction for intra-module memory allocator.
///< It supports allocating remote memory (network, GPU, etc) and
///< provides the means to copy remote memory to local. In order to
///< implement custom memory API the one needs to inherit from MemoryApi
///< and override basic XxxRaw methods.
///<
class MemoryApi
{
public:
    // Type of an api: either local or remote
    enum class ApiType
    {
        // Api producing local memory
        kLocal,
        // Api producing remote memory
        kRemote
    };
    
    // Default consructor
    MemoryApi(){}
    // Destructor
    virtual ~MemoryApi() = 0;
    // Get an id of a memory location managed by this instance of an API
    virtual LocationId  GetLocationId() const = 0;
    // Api type
    virtual ApiType     GetApiType() const = 0;
    
    // Non-virtual part
    template <typename T> std::unique_ptr<MemoryArea<T> > Allocate(size_t size, T const* init_data = nullptr);
    template <typename T> void Copy(MemoryArea<T>& dst, T const* src, size_t offset, size_t size);
    template <typename T> void Copy(T* dst, MemoryArea<T> const& src, size_t offset, size_t size);
    template <typename T> void Set(MemoryArea<T>& dst, T const& val,  size_t offset, size_t size);
    
protected:
    // Allocate memory area of a specified size
    virtual RawMemoryArea* AllocateRaw(size_t size, void const* init_data = nullptr) = 0;
    // Copy data from local memory buffer into memory area
    virtual void CopyRaw(RawMemoryArea& dst, void const* src, size_t offset, size_t size) = 0;
    // Copy data from memory area to local buffer
    virtual void CopyRaw(void* dst, RawMemoryArea const& src, size_t offset, size_t size) = 0;
    // Set memory area data to local value
    virtual void SetRaw(RawMemoryArea& dst, void const* pattern, size_t pattern_size, size_t offset, size_t repeat_count) = 0;

    // Forbidden stuff
    MemoryApi(MemoryApi const&);
    MemoryApi& operator = (MemoryApi const&);
};

inline MemoryApi::~MemoryApi()
{
}

template <typename T> std::unique_ptr<MemoryArea<T> > MemoryApi::Allocate(size_t size, T const* init_data)
{
    return std::unique_ptr<MemoryArea<T> >(new MemoryArea<T>(AllocateRaw(size * sizeof(T), init_data)));
}

template <typename T> void MemoryApi::Copy(MemoryArea<T>& dst, T const* src, size_t offset, size_t size)
{
    CopyRaw(dst, src, offset * sizeof(T), size * sizeof(T));
}

template <typename T> void MemoryApi::Copy(T* dst, MemoryArea<T> const& src, size_t offset, size_t size)
{
    CopyRaw(dst, src, offset * sizeof(T), size * sizeof(T));
}

template <typename T> void MemoryApi::Set(MemoryArea<T>& dst, T const& val, size_t offset, size_t size)
{
    SetRaw(dst, &val, sizeof(T), offset * sizeof(T), size);
}


#endif // MEMORYAPI_H
