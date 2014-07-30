#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "../intersection/intersection_api.h"
#include "../math/ray.h"
#include "../math/bbox.h"

class Primitive
{
public:

    struct Intersection
    {
        float3 p;
        float3 n;
        float2 t;
        int    m;
    };

    virtual ~Primitive(){}

    virtual bool Intersect(ray& r, Intersection& isect) const = 0;
    virtual bool Intersect(ray& r) const = 0;

    virtual bbox Bounds() const = 0;
};

#endif