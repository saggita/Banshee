#include "local_memory_api.h"
#include "local_memory_area.h"

#include <string.h>

RawMemoryArea* LocalMemoryApi::AllocateRaw(size_t size, void const* init_data)
{
    auto ptr = new LocalMemoryArea(*this, size);

    if (init_data)
    {
        CopyRaw(*ptr, init_data, 0, size);
    }
    
    return ptr;
}

void LocalMemoryApi::CopyRaw(RawMemoryArea& dst, void const* src, size_t offset, size_t size)
{
    memcpy((char*)dst.raw_ptr() + offset, src, size);
}

void LocalMemoryApi::CopyRaw(void* dst, RawMemoryArea const& src, size_t offset, size_t size)
{
    memcpy(dst, (char const*)src.raw_ptr() + offset, size);
}

//void LocalMemoryApi::Set(RawMemoryArea& dst, unsigned char val, size_t offset, size_t size)
//{
//    memset((char*)dst.ptr() + offset, val, size);
//}

LocationId LocalMemoryApi::GetLocationId() const
{
    // TODO: replace that later
    return 0;
}

MemoryApi::ApiType    LocalMemoryApi::GetApiType() const
{
    return ApiType::kLocal;
}