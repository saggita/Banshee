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
        // Transform object space bbox into world space
        float3 pmin(-radius_, -radius_, -radius_);
        float3 pmax( radius_,  radius_,  radius_);
        bounds_ = bbox(transform_point(pmin, worldmat_), transform_point(pmax, worldmat_));
    }

    // Intersection override
    bool Intersect(ray& r,  float& t, Intersection& isect) const;
    // Intersection check override
    bool Intersect(ray& r) const;
    // Bounding box override
    bbox const& Bounds() const;

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
    bbox bounds_;
};


#endif
