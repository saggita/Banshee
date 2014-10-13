#ifndef INTERSECTIONAPI_H
#define INTERSECTIONAPI_H

#include "../math/float3.h"
#include "../math/float2.h"
#include "../math/ray.h"

namespace FireRays {
    /// Represents a device, which can be used by API for intersection purposes.
    /// API is distributing the work across multiple devices itsels, so
    /// this structure is only used to query devices configuration and
    /// limit the number of devices available for the API.
    struct IntersectionDeviceInfo
    {
        // Device type
        enum Type
        {
            kCpu,
            kGpu,
            kAccelerator
        };
        
        // TODO: this is most likely not needed
        // API mask components
        enum Api
        {
            kNative        = 0x1,
            kOpenCl        = 0x2,
            kDirectCompute = 0x4,
            kMantle        = 0x8
        };
        
        // Device name
        char const* name;
        // Device vendor
        char const* vendor;
        // Device type
        Type type;
        // Supported APIs
        // TODO: this is most likely not needed !!
        int apis;
    };
    
    // Forward declaration of entities
    class Shape;
    class Mesh;
    class Instance;
    class Buffer;
    class Event;
    
    /// IntersectionApi is designed to provide fast means for ray-scene intersection
    /// for AMD architectures. It effectively absracts underlying AMD hardware and
    /// software stack and allows user to issue low-latency batched ray queries.
    /// The API has 2 major workflows:
    ///    - Fast path: the data is expected to be in host memory as well as the returned result is put there
    ///    - Complete path: the data can be put into remote memory allocated with API and can be accessed.
    ///      by the app directly in remote memory space.
    ///
    class IntersectionApi
    {
    public:
        /******************************************
                    Data types
         ******************************************/
        // A bitmask components for API constructing method
        enum ApiUsageHint
        {
            kDynamicGeometry = 0x1,
            kCoherentRays    = 0x2
        };
        
        // Describes a ray. Used in ray casting functionality.
        struct Ray;
        // Described an intersection.
        struct HitInfo;
        
        /******************************************
                     Device management
         ******************************************/
        // Use this part of API to query for available devices and
        // limit the set of devices which API is going to use
        
        // Get the number of devices available in the system
        static int GetIntersectionDeviceCount();
        // Get the information for the specified device
        static IntersectionDeviceInfo const& GetIntersectionDeviceInfo(int devidx);
        
        
        /******************************************
                  API lifetime management
         ******************************************/
        // usagehint is a bitmaks created with ApiUsageHint bitflags
        // By default a scene assumed to be static and expected to
        // be used for both coherent and incoherent ray tracing
        // TODO: probably accept maximum batch size here to choose for most efficient implementation
        // TODO: do we need a way to force to use say given OpenCL context??
        // TODO: how do we interop with an app which needs data in GPU memory? How to use the same OpenCL context?
        
        // Single device versions
        // THROWS:
        static IntersectionApi* Create(int usagehint);
        static IntersectionApi* Create(int usagehint, int device, int apimask);
        // Multi device version
        // THROWS:
        static IntersectionApi* Create(int usagehint, int const* devices, int const* apimasks, int numdevices);
        
        // Deallocation (to simplify DLL scenario)
        static void Delete(IntersectionApi* api);
        
        
        /******************************************
                  Geometry manipulation
         ******************************************/
        // Fast path functions to create entities from host memory
        
        // The mesh might be mixed quad\triangle mesh which is determined
        // by numfacevertices array containing numfaces entries describing
        // the number of vertices for current face (3 or 4)
        // The call is blocking, so the returned value is ready upon return.
        virtual Mesh*     CreateMesh(
                                     // Position data
                                     float* vertices, int voffset, int vstride,
                                     // Normal data
                                     float* normals, int noffset, int nstride,
                                     // UV
                                     float* uv, int uvoffset, int uvstride,
                                     // Index data for vertices
                                     int* vindices, int vioffset, int vistride,
                                     // Index data for normals
                                     int* nindices, int nioffset, int nistride,
                                     // Index data for uvs
                                     int* uvindices, int uvioffset, int uvistride,
                                     // Numbers of vertices per face
                                     int* numfacevertices,
                                     // Number of faces
                                     int  numface
                                     
                                     ) const = 0;
        
        // Create an instance of a shape with its own transform (set via Shape interface).
        // The call is blocking, so the returned value is ready upon return.
        virtual Instance* CreateInstance(Shape const* shape) const = 0;
        // Delete the shape (to simplify DLL boundary crossing
        virtual void DeleteShape(Shape* shape) = 0;
        // Attach shape to participate in intersection process
        virtual void AttachShape(Shape const* shape) = 0;
        // Detach shape, i.e. it is not going to be considered part of the scene anymore
        virtual void DetachShape(Shape const* shape) = 0;
        // Commit all geometry creations/changes
        virtual void Commit() = 0;
        
        /******************************************
                    Memory management
         ******************************************/
        // Create a buffer to use the most efficient acceleration possible
        // TODO: Need to account for rear/write access
        virtual Buffer* CreateBuffer(size_t size, void* initdata) const = 0;
        // Delete the buffer
        virtual void DeleteBuffer(Buffer* buffer) = 0;
        // Map buffer
        // TODO: do we need read\write access?
        // TODO: Nick: probably need an async map not to break the submission, map discard
        // event pointer might be nullptr.
        // The call is asynchronous.
        virtual void MapBuffer(Buffer const* buffer, size_t offset, size_t size, void** data, Event** event) const = 0;
        // Unmap buffer
        virtual void UnmapBuffer(Buffer const* buffer) = 0;
        
        /******************************************
                      Ray casting
         ******************************************/
        // Fast path:
        // Find closest intersection.
        // TODO: do we need to provide SoA version to be able to fetch for SSE directly?
        // TODO: do we need to modify rays' intersection range?
        // The call is blocking.
        virtual void IntersectBatch(Ray const* rays, int numrays, int* hitresults, HitInfo* hitinfos) const = 0;
        // Find any intersection.
        // The call is blocking.
        virtual void IntersectBatch(Ray const* rays, int numrays, int* hitresults) const = 0;
        
        // Complete path:
        // Find closest intersection
        // TODO: do we need to modify rays' intersection range?
        // TODO: SoA vs AoS?
        // The call is asynchronous. Event pointer might be nullptr.
        virtual void IntersectBatch(Buffer const* rays, int numrays, Buffer* hitresults, Buffer* hitinfos, Event** event) const = 0;
        // Find any intersection.
        // The call is asynchronous. Event pointer might be nullptr.
        virtual void IntersectBatch(Buffer const* rays, int numrays, Buffer* hitresults, Event** event) const = 0;
        
    
        /******************************************
                   Experimental part
         ******************************************/
        
        
    protected:
        IntersectionApi();
        IntersectionApi(IntersectionApi const&);
        IntersectionApi& operator = (IntersectionApi const&);
        virtual ~IntersectionApi() = 0;
    };
    
    struct IntersectionApi::Ray
    {
        // Ray origin
        float3 o;
        // Ray direction
        float3 d;
        // Parametric range
        float2 range;
        // Time
        float  time;
    };
    
    struct IntersectionApi::HitInfo
    {
        // Parametric coordinates of a hit
        float2 uv;
        // Shape data specified in Shape's SetUserData call
        int shapeuserdata;
        // Primitive index within a shape
        int primidx;
    };
    
    inline IntersectionApi::~IntersectionApi(){}
}


#endif // INTERSECTIONAPI_H
