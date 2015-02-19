/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/

#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <vector>

#include "../math/ray.h"
#include "../math/bbox.h"

class AreaLight;

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
        // Primitive
        Primitive const* primitive;
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
    
    // Constructor
    Primitive() : arealight_(nullptr){}

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
    // The sample point is expected to be uniform over the area and pdf is returned
    // with respect to surface area(!)
    virtual void Sample(float2 const& sample, SampleData& sampledata, float& pdf) const { return; }
    // Each primitive with an area > 0 is required to be able to provide sample points on its surface.
    // This method returns a sample point for a given sample in world space.
    // The sample point is expected to be uniform and PDF is converted to solid angle
    // substanded by the shape as visible from point p.
    virtual void Sample(float3 const& p, float2 const& sample, SampleData& sampledata, float& pdf) const { return; }
    // Each primitive with an area > 0 is required to be able to provide sample points on its surface.
    // The methods provides PDF value of an event that w direction is sampled from p
    // while sampling this primitive
    virtual float Pdf(float3 const& p, float3 const& w) const { return 0.f; }
    // The method is in charge of splitting primitive bounding box with axis-aligned plane.
    // Default implementation simply linearly interpolates between min and max points.
    // Overrides are expected to provide more preciese bounds.
    // true indicates that the bounds was split, otherwise the plane misses it
    virtual bool SplitBounds(int axis, float border, bbox& leftbounds, bbox& rightbounds) const;
    
    
    // Primitive might be linked to some AreaLight object if it has emissive material
    AreaLight* arealight_;
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