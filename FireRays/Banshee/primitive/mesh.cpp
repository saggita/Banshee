#include "mesh.h"

Mesh::Mesh(float const* vertices, int vnum, int vstride,
           float const* normals, int nnum, int nstride,
           float const* uvs, int unum, int ustride,
           int const* vidx, int vistride,
           int const* nidx, int nistride,
           int const* uidx, int uistride,
           int const* materials, int mstride,
           int nfaces,
           matrix const& wm, matrix const& wmi)
           : TransformablePrimitive(wm, wmi)
{
    /// Handle vertices
    vertices_.resize(vnum);
    vstride = (vstride == 0)?(3 * sizeof(float)) : vstride;

    float3 temp;
    for (int i=0; i<vnum; ++i)
    {
        float const* current = (float const*)((char*)vertices + i*vstride);
        temp.x = current[0];
        temp.y = current[1];
        temp.z = current[2];
        vertices_[i] = temp;
    }

    /// Handle normals
    normals_.resize(nnum);
    nstride = (nstride == 0)?(3 * sizeof(float)) : nstride;

    for (int i=0; i<nnum; ++i)
    {
        float const* current = (float const*)((char*)normals + i*nstride);
        temp.x = current[0];
        temp.y = current[1];
        temp.z = current[2];
        normals_[i] = temp;
    }

    // Handle UVs
    // Check if UVs are passed
    // If not use (0,0) uv for all
    bool hasuv = uvs && unum > 0;

    if (hasuv)
    {
        uvs_.resize(unum);
        ustride = (ustride == 0)?(2 * sizeof(float)) : ustride;

        for (int i=0; i<unum; ++i)
        {
            float const* current = (float const*)((char*)uvs + i*ustride);
            uvs_[i] = float2(current[0], current[1]);
        }
    }
    else
    {
        uvs_.push_back(float2(0,0));
    }

    vistride = (vistride == 0) ? sizeof(int) : vistride;
    nistride = (nistride == 0) ? sizeof(int) : nistride;
    uistride = (uistride == 0) ? sizeof(int) : uistride;

    /// Construct triangles
    triangles_.resize(nfaces);
    for (int i = 0; i < nfaces; ++i)
    {
        int vi0 = *((int const*)((char*)vidx + 3 * i * vistride));
        int vi1 = *((int const*)((char*)vidx + (3 * i + 1) * vistride));
        int vi2 = *((int const*)((char*)vidx + (3 * i + 2) * vistride));

        int ni0 = *((int const*)((char*)nidx + 3 * i * nistride));
        int ni1 = *((int const*)((char*)nidx + (3 * i + 1) * nistride));
        int ni2 = *((int const*)((char*)nidx + (3 * i + 2) * nistride));

        int ui0 = 0;
        int ui1 = 0;
        int ui2 = 0;

        if (hasuv)
        {
            ui0 = *((int const*)((char*)uidx + 3 * i * uistride));
            ui1 = *((int const*)((char*)uidx + (3 * i + 1) * uistride));
            ui2 = *((int const*)((char*)uidx + (3 * i + 2) * uistride));
        }

        int m = *((int const*)((char*)materials + i * mstride));
        triangles_[i].reset(new IndexedTriangle(vi0, vi1, vi2, ni0, ni1, ni2, ui0, ui1, ui2, m, *this));
    }

    // Update mesh bounding box
    CalcBounds();
}

bbox Mesh::Bounds() const
{
    return bounds_;
}

void Mesh::Refine (std::vector<Primitive*>& prims)
{
    // TODO: optimize memory management later
    for (int i=0; i<triangles_.size();++i)
    {
        prims.push_back(triangles_[i].get());
    }
}

void Mesh::CalcBounds()
{
    for (int i=0; i<triangles_.size();++i)
    {
        bounds_ = bboxunion(bounds_, triangles_[i]->Bounds());
    }
}
