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

#ifndef SHAPE_BUNDLE
#define SHAPE_BUNDLE

#include <vector>
#include <cstddef>

#include "../math/ray.h"
#include "../math/bbox.h"
#include "../math/matrix.h"

class AreaLight;

///< ShapeBundle is a container of individual intersectable objects which doesn't make
///< sense to devote a separate class to (like a triangle mesh)
///<
class ShapeBundle
{
public:
    
    // Hit information
    // returned by intersection routines
    struct Hit;

    // Sample information
    // returned by sampling routines
    struct Sample;
    
    // Constructor
    ShapeBundle();
    
    // Destructor
    virtual ~ShapeBundle() = 0;
    
    // Number of shapes in the bundle
    virtual std::size_t GetNumShapes() const = 0;
    
    // Test shape number idx against the ray
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // REQUIRED: hit.t initialized for some value which is used as a current closest hit distance
    // CONTRACT:   true is returned if r intersects shape idx with hit distance closer to hit.t
    //           hit.t updated accordingly, other data in hit updated accordingly
    //           false if no hits found, hit is unchanged in this case
    //
    virtual bool IntersectShape(std::size_t idx, ray const& r, Hit& hit) const = 0;
    
    // Test shape number idx against the ray
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: true is returned if r intersects shape idx, false otherwise
    //
    virtual bool IntersectShape(std::size_t idx, ray const& r) const = 0;
    
    // Get shape number idx world bounding box
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: Bounding box returned
    //
    virtual bbox GetShapeWorldBounds(std::size_t idx) const = 0;
    
    // Get shape number idx object bounding box
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: Bounding box returned
    //
    virtual bbox GetShapeObjectBounds(std::size_t idx) const = 0;
    
    // Get shape number idx surface area
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: Surface area returned
    //
    virtual float GetShapeSurfaceArea(std::size_t idx) const = 0;
    
    // Sample point on idx shape
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // REQUIRED: uv belongs to [0..1]x[0..1], otherwise effect undefined
    // CONTRACT: The method fills int sample data and provides PDF for the sample,
    //           note that PDF might be 0 in which case data is not filled in.
    // IMPORTANT: PDF returned by the method with regards to surface area.
    //
    virtual void GetSampleOnShape(std::size_t idx, float2 const& uv, Sample& sample) const;
    
    // Sample point on idx shape from point p
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // REQUIRED: uv belongs to [0..1]x[0..1], otherwise effect undefined
    // CONTRACT: The method fills int sample data and provides PDF for the sample
    //           note that PDF might be 0 in which case data is not filled in.
    // IMPORTANT: PDF returned by the method with regards to solid angle subtended by shape
    //           at the point p.
    //
    virtual void GetSampleOnShape(std::size_t idx, float3 const& p, float2 const& uv, Sample& sample) const;
    
    // Get PDF value of a particular direction from a point p
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: The method retuns PDF of w direction with respect solid angle subtended by shape
    //           at the point p.
    //
    virtual float GetPdfOnShape(std::size_t idx, float3 const& p, float3 const& w) const;
    
    // Get world space bounding box of the whole bundle
    virtual bbox GetWorldBounds() const;
    
    // Get world space bounding box of the whole bundle
    virtual bbox GetObjectBounds() const;
    
    // If the object is area light return area light interface for it.
    virtual AreaLight const* GetAreaLight() const;
    
    // Set transform on a bundle
    void SetTransform(matrix const& m, matrix const& minv);
    
    // Get transform
    void GetTransform(matrix& m, matrix& minv) const;
    
private:
    // ShapeBundle might be linked to some AreaLight object if it has emissive material
    AreaLight* arealight_;
    // ShapeBundle can have a transform
    matrix worlmat_;
    matrix worldmatinv_;
    
    friend class AreaLight;
};


// Hit information
struct ShapeBundle::Hit
{
    // Parametric distance
    float t;
    // World space position
    float3 p;
    // World space shading normal
    float3 n;
    // World space geometric normal
    float3 ng;
    // Tangent
    float3 dpdu;
    // Bitangent
    float3 dpdv;
    // UV parametrization
    float2 uv;
    // Material index
    int m;
    // Primitive
    ShapeBundle const* bundle;
};

// Sample information
struct ShapeBundle::Sample
{
    // Sample pdf
    float pdf;
    // World space position
    float3 p;
    // World space normal
    float3 n;
    // UV parametrization
    float2 uv;
    // Material index
    int m;
    
    Sample()
    : pdf(0.f)
    , m(-1)
    {
    }
    
    Sample(Hit const& hit)
    : pdf(0.f)
    , p(hit.p)
    , n(hit.n)
    , uv(hit.uv)
    , m(hit.m)
    {
    }
};

inline ShapeBundle::ShapeBundle()
: arealight_(nullptr)
{
}

inline ShapeBundle::~ShapeBundle()
{
}

inline AreaLight const* ShapeBundle::GetAreaLight() const
{
    return arealight_;
}

inline bbox ShapeBundle::GetWorldBounds() const
{
    bbox res;
    for(size_t i = 0; i < GetNumShapes(); ++i)
    {
        res.grow(GetShapeWorldBounds(i));
    }
    
    return res;
}

inline bbox ShapeBundle::GetObjectBounds() const
{
    bbox res;
    for(size_t i = 0; i < GetNumShapes(); ++i)
    {
        res.grow(GetShapeObjectBounds(i));
    }
    
    return res;
}

inline void ShapeBundle::GetSampleOnShape(std::size_t idx, float2 const& uv, Sample& sample) const
{
    sample.pdf = 0.f;
    return;
}

inline void ShapeBundle::GetSampleOnShape(std::size_t idx, float3 const& p, float2 const& uv, Sample& sample) const
{
    sample.pdf = 0.f;
    return;
}

inline float ShapeBundle::GetPdfOnShape(std::size_t idx, float3 const& p, float3 const& w) const
{
    return 0.f;
}

inline void ShapeBundle::SetTransform(matrix const& m, matrix const& minv)
{
    worlmat_ = m;
    worldmatinv_ = minv;
}

inline void ShapeBundle::GetTransform(matrix& m, matrix& minv) const
{
    m = worlmat_;
    minv = worldmatinv_;
}

#endif // SHAPE_BUNDLE