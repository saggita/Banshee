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
#include "transformable_primitive.h"
#include "indexed_triangle.h"

///< Transformable primitive implementation which represents
///< triangle mesh. Vertices, normals and uvs are indixed separately
///< using their own index buffers each.
///<
class Mesh : public TransformablePrimitive
{
public:
    Mesh(float const* vertices, int vnum, int vstride,
         float const* normals, int nnum, int nstride,
         float const* uvs, int unum, int ustride,
         int const* vidx, int vistride,
         int const* nidx, int nistride,
         int const* uidx, int uistride,
         int const* materials, int mstride,
         int nfaces,
         matrix const& wm = matrix(), matrix const& wmi = matrix());

    // Intersection test
    bool Intersect(ray& r, float& t, Intersection& isect) const { return false; }
    // Intersection check test
    bool Intersect(ray& r) const { return false; }
    // World space bounding box
    bbox Bounds() const;
    // Intersectable flag: determines whether the primitive is
    // capable of direct intersection evaluation
    // By default it returns true
    bool intersectable() const { return false; }
    // If the shape is not intersectable, the following method is 
    // supposed to break it into parts (which might or might not be intersectable themselves)
    // Note that memory of the parts is owned by the primitive 
    void Refine (std::vector<Primitive*>& prims);

private:
    void CalcBounds();

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
    std::vector<std::unique_ptr<IndexedTriangle> > triangles_;
    /// Cached bbox
    bbox bounds_;
    /// Friend IndexedTriangle to allow data access
    friend class IndexedTriangle;
};

#endif // MESH_H
