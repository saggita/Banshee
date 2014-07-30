#ifndef INDEXED_TRIANGLE_H
#define INDEXED_TRIANGLE_H

#include "primitive.h"
#include "../math/float3.h"
#include "../math/float2.h"
#include "../math/bbox.h"

class IndexedTriangle : public Primitive
{
public:
    IndexedTriangle(unsigned pidx1, unsigned pidx2, unsigned pidx3,
                    unsigned nidx1, unsigned nidx2, unsigned nidx3,
                    unsigned tidx1, unsigned tidx2, unsigned tidx3,
                    unsigned m, float3* p, float3* n, float2* t)
                    : pidx1_(pidx1), pidx2_(pidx2), pidx3_(pidx3)
                    , nidx1_(nidx1), nidx2_(nidx2), nidx3_(nidx3)
                    , tidx1_(tidx1), tidx2_(tidx2), tidx3_(tidx3)
                    , m_(m), p_(p), n_(n), t_(t)
    {
    }

    bool Intersect(ray& r, Intersection& isect) const;
    bool Intersect(ray& r) const;

    bbox Bounds() const;

private:
    unsigned pidx1_, pidx2_, pidx3_;
    unsigned nidx1_, nidx2_, nidx3_;
    unsigned tidx1_, tidx2_, tidx3_;
    unsigned m_;

    float3* p_;
    float3* n_;
    float2* t_;
};

#endif // INDEXED_TRIANGLE_H    