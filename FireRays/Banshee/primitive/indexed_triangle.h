#ifndef INDEXED_TRIANGLE_H
#define INDEXED_TRIANGLE_H

#include "primitive.h"
#include "../math/float3.h"
#include "../math/float2.h"
#include "../math/bbox.h"

class Mesh;

///< IndexedTriangle stores indices of the position/normal/uv data
///< along with a pointers to their storages
///<
class IndexedTriangle : public Primitive
{
public:
    // Constructor
    IndexedTriangle(unsigned pidx1, unsigned pidx2, unsigned pidx3,
                    unsigned nidx1, unsigned nidx2, unsigned nidx3,
                    unsigned tidx1, unsigned tidx2, unsigned tidx3,
                    unsigned m, Mesh const& mesh)
                    : pidx1_(pidx1), pidx2_(pidx2), pidx3_(pidx3)
                    , nidx1_(nidx1), nidx2_(nidx2), nidx3_(nidx3)
                    , tidx1_(tidx1), tidx2_(tidx2), tidx3_(tidx3)
                    , m_(m), mesh_(mesh)
    {
        bounds_ = CalcBounds();
        CalcWorldSpacePositions();
    }

    // Intersection override
    bool Intersect(ray& r, float& t, Intersection& isect) const;
    // Intersection check override
    bool Intersect(ray& r) const;
    // Bounding box override
    bbox Bounds() const { return bounds_; }

    // Calculate a sample point on the surface of a triangle
    void Sample(float2 const& sample, SampleData& sampledata, float& pdf) const;

    // Surface area of a triangle
    float surface_area() const;

    // The method is in charge of splitting primitive bounding box with axis-aligned plane.
    // Default implementation simply linearly interpolates between min and max points.
    // Overrides are expected to provide more preciese bounds.
    // true indicates that the bounds was split, otherwise the plane misses it
    bool SplitBounds(int axis, float border, bbox& leftbounds, bbox& rightbounds) const;

private:
    // Update bounds
    bbox CalcBounds();
    // Updated cached world space positions
    void CalcWorldSpacePositions();
    // Data indices
    unsigned pidx1_, pidx2_, pidx3_;
    unsigned nidx1_, nidx2_, nidx3_;
    unsigned tidx1_, tidx2_, tidx3_;
    // Material index
    unsigned m_;

    // Pointer to parent mesh
    Mesh const& mesh_;
    // Cached bounds
    bbox bounds_;
    // Cached world space positions
    float3 wp[3];
};

#endif // INDEXED_TRIANGLE_H    