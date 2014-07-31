#ifndef SPHERE_H
#define SPHERE_H

#include "primitive.h"
#include "../math/float3.h"
#include "../math/float2.h"
#include "../math/bbox.h"


///< Shpere primitive implementation
///<
class Sphere: public Primitive
{
public:
    Sphere(float r = 1.f)
    : radius_(r)
    {
    }
    
    // Intersection override
    bool Intersect(ray& r, Intersection& isect) const;
    // Intersection check override
    bool Intersect(ray& r) const;
    // Bounding box override
    bbox Bounds() const;
    
private:
    float radius_;
};


#endif
