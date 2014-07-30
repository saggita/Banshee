#ifndef INTERSECTIONAPI_H
#define INTERSECTIONAPI_H

/// MemoryArea is the main external data exchange class
/// All APIs receive data chunks in the form of memory areas
template <typename T> class MemoryArea;

///< IntersectionApi defines and interface for intersection engines
///< it supports two types of operations:
///<    - Async data uploading (includes preparation for isect work)
///<    - Async intersection queries (ray vs primitive)
///<
class IntersectionApi
{
public:
    
    ///< CompletionEvent is supposed to provide the means for asynchronous
    ///< execution for IntersectionApi operations
    ///<
    class CompletionEvent
    {
    public:
        ///
        virtual ~CompletionEvent() = 0;
        /// Blocking wait operation
        virtual void Wait() = 0;
        /// Completeness check
        virtual bool Complete() = 0;
    };
    
    // Default constructor
    IntersectionApi(){}
    
    // Destructor
    virtual ~IntersectionApi() = 0;
    
    // Upload mesh into the API and prepare for intersection tasks
    // The caller must ensure the previous call to UploadMesh has finished
    // prior to calling the function again. Otherwise the behavior is undefined.
    std::unique_ptr<CompletionEvent> UploadMesh(MemoryArea<float>& vertices, unsigned num_vertices, unsigned vstride,
                                                MemoryArea<float>& normals,  unsigned num_normals,  unsigned nstride,
                                                MemoryArea<float>& uv,       unsigned num_uvs,      unsigned uvstride,
                                                MemoryArea<int>& vertex_indices, unsigned vidx_stride,
                                                MemoryArea<int>& normal_indices, unsigned nidx_stride,
                                                MemoryArea<int>& uv_indices,     unsigned uidx_stride,
                                                MemoryArea<int>& materials,
                                                unsigned num_faces) = 0;
    
    
    ///< Intersection description
    ///<
    struct Intersection
    {
        // World space position
        float3 p;
        // World space normal
        float3 n;
        // Texture coordinate
        float2 uv;
        // Material index
        int    m;
    };
    
    ///< Ray description
    ///<
    struct Ray
    {
        // Origin
        float3 o;
        // Normalized direction
        float3 d;
        // Start and end parametric values;
        float2 t;
    };
    
    std::unique_ptr<CompletionEvent> CastIndividual(MemoryArea<Ray>& ray, MemoryArea<Intersection>& isect) = 0;
    std::unique_ptr<CompletionEvent> CastBatch(MemoryArea<Ray>& rays, int batch_size, MemoryArea<Intersection>& isect) = 0;
    std::unique_ptr<CompletionEvent> CastBatch(MemoryArea<Ray>& rays, MemoryArea<int> batch_size, MemoryArea<Intersection>& isect) = 0;
    
    
protected:
    IntersectionApi(IntersectionApi const&);
    IntersectionApi& operator = (IntersectionApi const&);
};

///
inline IntersectionApi::~IntersectionApi()
{
}


#endif // INTERSECTIONAPI_H
