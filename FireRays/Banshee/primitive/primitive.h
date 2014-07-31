#ifndef PRIMITIVE_H
#define PRIMITIVE_H

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
        // Texture coordinate
        float2 t;
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
};

#endif