#ifndef MEMORYAREA_H
#define MEMORYAREA_H

class MemoryApi;

///< RawMemoryArea is a part of MemoryApi subsystem.
///< It represents a chunk of memory from either 
///< local or remote location. RawMemoryArea is typically
///< implemented along with particular MemoryApi impl.
///<
class RawMemoryArea
{
public:
    // Default cobstructor
    RawMemoryArea(){}

    // Destructor
    virtual         ~RawMemoryArea() = 0;

    // Get local pointer (only for LOCAL buffers)
    virtual void*       raw_ptr() = 0;
    virtual void const* raw_ptr() const = 0;

    // Get memory area size
    virtual size_t      size() const = 0;

    // Get API that produced this area
    virtual MemoryApi&          GetApi() const = 0;

protected:
    RawMemoryArea(RawMemoryArea const&);
    RawMemoryArea& operator = (RawMemoryArea const&);
};

inline RawMemoryArea::~RawMemoryArea(){}

// Wrapper for memory area to establish type-safety checks
template <typename T>
class MemoryArea : public RawMemoryArea
{
public:
    MemoryArea(RawMemoryArea* raw_memory_area)
    : raw_memory_area_(raw_memory_area)
    {}

    ~MemoryArea()
    {
        delete raw_memory_area_;
    }

    T*       ptr()          { return static_cast<T*>(raw_memory_area_->raw_ptr()); }
    T const* ptr() const    {return static_cast<T const*>(raw_memory_area_->raw_ptr()); }

    void*       raw_ptr()       { return raw_memory_area_->raw_ptr(); }
    void const* raw_ptr() const {return raw_memory_area_->raw_ptr(); }

    // Get memory area size
    virtual size_t      size() const { return raw_memory_area_->size() / sizeof(T); }

    // Get API that produced this area
    MemoryApi&          GetApi() const { return raw_memory_area_->GetApi(); }

private:
    MemoryArea(MemoryArea<T> const&);
    MemoryArea<T>& operator = (MemoryArea<T> const&);
    
    RawMemoryArea* raw_memory_area_;
};

#endif //MEMORYAREA_H
