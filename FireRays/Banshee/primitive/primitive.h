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

    struct SampleData
    {
        // World space position
        float3 p;
        // World space normal
        float3 n;
        // UV parametrization
        float2 uv;

        SampleData()
        {
        }
        SampleData(Intersection const& isect)
            : p(isect.p)
            , n(isect.n)
            , uv(isect.uv)
        {
        }
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
    // Surface area of the primitive
    // TODO: should that be pure?
    virtual float surface_area() const { return 0.f; };
    // Each primitive with an area > 0 is required to be able to provide sample points on its surface.
    // This method returns a sample point for a given sample in world space.
    virtual void Sample(float2 const& sample, SampleData& sampledata, float& pdf) const { return; }
    // The method is in charge of splitting primitive bounding box with axis-aligned plane.
    // Default implementation simply linearly interpolates between min and max points.
    // Overrides are expected to provide more preciese bounds.
    // true indicates that the bounds was split, otherwise the plane misses it
    virtual bool SplitBounds(int axis, float border, bbox& leftbounds, bbox& rightbounds) const;
};

inline bool Primitive::SplitBounds(int axis, float border, bbox& leftbounds, bbox& rightbounds) const
{
    bbox bounds = Bounds();
    if (border <= bounds.pmax[axis] && border >= bounds.pmin[axis])
    {
        // Copy min and max values
        leftbounds.pmin = rightbounds.pmin = bounds.pmin;
        leftbounds.pmax = rightbounds.pmax = bounds.pmax;

        // Break now
        leftbounds.pmax[axis] = border;
        rightbounds.pmin[axis] = border;

        return true;
    }

    return false;
}


#endif