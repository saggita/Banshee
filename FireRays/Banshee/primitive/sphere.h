#ifndef SPHERE_H
#define SPHERE_H

#include "transformable_primitive.h"
#include "../math/mathutils.h"
#include "../math/matrix.h"
#include "../math/float3.h"
#include "../math/float2.h"
#include "../math/bbox.h"


///< Shpere primitive implementation
///<
class Sphere: public TransformablePrimitive
{
public:
    Sphere(float r = 1.f, matrix const& wm = matrix(), matrix const& wmi = matrix(), int m = 0)
    : TransformablePrimitive(wm, wmi)
    , radius_(r)
    , m_(m)
    {
    }

    // Intersection override
    bool Intersect(ray& r,  float& t, Intersection& isect) const;
    // Intersection check override
    bool Intersect(ray& r) const;
    // Bounding box override
    bbox Bounds() const;

    // Calculate a sample point on the surface of a sphere
    void Sample(float2 const& sample, SampleData& sampledata, float& pdf) const;

    // Surface area of a sphere;
    float surface_area() const;

private:
    // Fill Intersection structure with the data
    // corresponding to passed object space point p
    void FillIntersectionInfo(float3 const& p, Intersection& isect) const;

    float  radius_;
    int m_;
};


#endif
