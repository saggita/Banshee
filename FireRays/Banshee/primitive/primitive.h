#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <vector>

#include "../intersection/intersection_api.h"
#include "../math/ray.h"
#include "../math/bbox.h"

///< Base class for all CPU-based geometric primitives.
///< Override methods to add new geometry types
///<
class Primitive
{
public:

    struct Intersection
    {
        // World space position
        float3 p;
        // World space normal
        float3 n;
        // Tangent
        float3 dpdu;
        // Bitangent
        float3 dpdv;
        // UV parametrization
        float2 uv;
        // Material index
        int    m;
    };

    // Destructor
    virtual ~Primitive(){}

    // Intersection test
    virtual bool Intersect(ray& r, float& t, Intersection& isect) const = 0;
    // Intersection check test
    virtual bool Intersect(ray& r) const = 0;
    // World space bounding box
    virtual bbox Bounds() const = 0;
    // Intersectable flag: determines whether the primitive is
    // capable of direct intersection evaluation
    // By default it returns true
    virtual bool intersectable() const { return true; }
    // If the shape is not intersectable, the following method is 
    // supposed to break it into parts (which might or might not be intersectable themselves)
    // Note that memory of the parts is owned by the primitive 
    virtual void Refine (std::vector<Primitive*>& prims) { }
};

#endif