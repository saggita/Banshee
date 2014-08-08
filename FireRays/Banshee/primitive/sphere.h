#ifndef SPHERE_H
#define SPHERE_H

#include "primitive.h"
#include "../math/matrix.h"
#include "../math/float3.h"
#include "../math/float2.h"
#include "../math/bbox.h"


///< Shpere primitive implementation
///<
class Sphere: public Primitive
{
public:
    Sphere(float r = 1.f, matrix const& wm = matrix(), matrix const& wmi = matrix(), int m = 0)
    : radius_(r)
    , worldmat_(wm)
    , worldmatinv_(wmi)
    , m_(m)
    {
    }

    // Intersection override
    bool Intersect(ray& r,  float& t, Intersection& isect) const;
    // Intersection check override
    bool Intersect(ray& r) const;
    // Bounding box override
    bbox Bounds() const;

private:
    // Fill Intersection structure with the data
    // corresponding to passed object space point p
    void FillIntersectionInfo(float3 const& p, Intersection& isect) const;

    float  radius_;
    matrix worldmat_;
    matrix worldmatinv_;
    int m_;
};


#endif
