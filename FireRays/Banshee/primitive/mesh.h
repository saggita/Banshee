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

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <memory>

#include "../math/float3.h"
#include "../math/float2.h"
#include "shapebundle.h"

///< Transformable primitive implementation which represents
///< triangle mesh. Vertices, normals and uvs are indixed separately
///< using their own index buffers each.
///<
class Mesh : public ShapeBundle
{
public:
    /**
     Data types
     */
    // Mesh faces
    // Keeps indices for all components
    // and material index
    struct Face
    {
        int vi0, vi1, vi2;
        int ni0, ni1, ni2;
        int ti0, ti1, ti2;
        int m;
    };
    
    /**
     Mesh methods
     */
    // Constructor
    Mesh(float const* vertices, int vnum, int vstride,
         float const* normals, int nnum, int nstride,
         float const* uvs, int unum, int ustride,
         int const* vidx, int vistride,
         int const* nidx, int nistride,
         int const* uidx, int uistride,
         int const* materials, int mstride,
         int nfaces);
    
    // Fill hit information (normal. uv, etc)
    // REQUIRED: IntersectFace(face, ro, t, a, b) == true
    void FillHit(size_t idx, float t, float a, float b, Hit& hit) const;
    
    //
    float3 const* GetVertices() const;
    //
    size_t GetNumVertices() const;
    //
    Face const* GetFaces() const;
    //
    size_t GetNumFaces() const;
    
    /** 
     ShapeBundle overrides
     */
    
    // Number of shapes in the bundle
    std::size_t GetNumShapes() const { return faces_.size(); }
    
    // Test shape number idx against the ray
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // REQUIRED: hit.t initialized for some value which is used as a current closest hit distance
    // CONTRACT:   true is returned if r intersects shape idx with hit distance closer to hit.t
    //           hit.t updated accordingly, other data in hit updated accordingly
    //           false if no hits found, hit is unchanged in this case
    //
    bool IntersectShape(std::size_t idx, ray const& r, Hit& hit) const;
    
    // Test shape number idx against the ray
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: true is returned if r intersects shape idx, false otherwise
    //
    bool IntersectShape(std::size_t idx, ray const& r) const;
    
    // Get shape number idx world bounding box
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: Bounding box returned
    //
    bbox GetShapeWorldBounds(std::size_t idx) const;
    
    // Get shape number idx object bounding box
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: Bounding box returned
    //
    bbox GetShapeObjectBounds(std::size_t idx) const;
    
    // Get shape number idx surface area
    // REQUIRED: 0 <= idx < GetNumShapes(), otherwise effect undefined
    // CONTRACT: Surface area returned
    //
    float GetShapeSurfaceArea(std::size_t idx) const;
    
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
    
protected:
    // Test face against a given ray returning barycentric coords of a hit
    // and ray hit distance
    bool IntersectFace(Face const& face, ray const& ro, float tmax, float& t, float& a, float& b) const;
    // Fill hit information (normal. uv, etc)
    // REQUIRED: IntersectFace(face, ro, t, a, b) == true
    void FillHit(Face const& face, float t, float a, float b, Hit& hit) const;
    // Fill sample information
    void FillSample(Face const& face, float a, float b, Sample& sample) const;

private:

    /// Disallow to copy meshes, too heavy
    Mesh(Mesh const& o);
    Mesh& operator = (Mesh const& o);

    /// Vertices
    std::vector<float3> vertices_;
    /// Normals
    std::vector<float3> normals_;
    /// UVs
    std::vector<float2> uvs_;
    /// Triangles
    std::vector<Face> faces_;
};

#endif // MESH_H
