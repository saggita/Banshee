#ifndef MESH_H
#define MESH_H

#include <vector>

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
