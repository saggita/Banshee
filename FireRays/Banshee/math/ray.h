#ifndef RAY_H
#define RAY_H

#include <limits>

#include "float3.h"
#include "float2.h"

class ray
{
public:
    ray(float3 const& oo = float3(0,0,0), float3 const& dd = float3(0,0,0), float2 const& rng = float2(0, std::numeric_limits<float>::max()))
        : o(oo)
        , d(dd)
        , t(rng)
        , id(0)
    {
    }

    float3 operator ()(float t) const
    {
        return o + t * d;
    }

    float3 o;
    float3 d;
    float2 t;
    int   id;
};

#endif // RAY_H