#include "mesh.h"

#include "../math/mathutils.h"

#include <cassert>

bool Mesh::IntersectFace(Face const& face, ray const& ro, float tmax, float& t, float& a, float& b) const
{
    float3 const& p1(vertices_[face.vi0]);
    float3 const& p2(vertices_[face.vi1]);
    float3 const& p3(vertices_[face.vi2]);
    
    float3 e1 = p2 - p1;
    float3 e2 = p3 - p1;
    
    float3 s1 = cross(ro.d, e2);
    float  invd = 1.f/dot(s1, e1);
    
    float3 d = ro.o - p1;
    float  b1 = dot(d, s1) * invd;
    
    if (b1 < 0.f || b1 > 1.0)
        return false;
    
    float3 s2 = cross(d, e1);
    float  b2 = dot(ro.d, s2) * invd;
    
    if (b2 < 0.f || b1 + b2 > 1.f)
        return false;
    
    float tmp = dot(e2, s2) * invd;
    
    if (tmp > ro.t.x && tmp <= tmax)
    {
        t = tmp;
        
        a = b1;
        
        b = b2;
        
        return true;
    }
    
    return false;
}

void Mesh::FillHit(Face const& face, float t, float a, float b, Hit& hit) const
{
    // Get transform
    matrix m, minv;
    GetTransform(m, minv);
    
    float3 const& p1(vertices_[face.vi0]);
    float3 const& p2(vertices_[face.vi1]);
    float3 const& p3(vertices_[face.vi2]);
    
    float3 const& n1(normals_[face.ni0]);
    float3 const& n2(normals_[face.ni1]);
    float3 const& n3(normals_[face.ni2]);
     
    float2 const& t1(uvs_[face.ti0]);
    float2 const& t2(uvs_[face.ti1]);
    float2 const& t3(uvs_[face.ti2]);
    
    hit.t = t;
    hit.p = transform_point((1.f - a - b) * p1 + a * p2 + b * p3, m);
    hit.n = normalize(transform_normal((1.f - a - b) * n1 + a * n2 + b * n3, minv));
    hit.ng = normalize(transform_normal(cross(p3-p1, p2-p1), minv));
    if (dot(hit.n, hit.ng) < 0) hit.ng = -hit.ng;
     
     // Account for backfacing normal
     float du1 = t1.x - t3.x;
     float du2 = t2.x - t3.x;
     float dv1 = t1.y - t3.y;
     float dv2 = t2.y - t3.y;
     
     float3 dp1 = p1 - p3;
     float3 dp2 = p2 - p3;
     
     float det = du1 * dv2 - dv1 * du2;
    
     if (det != 0.f)
     {
         float invdet = 1.f / det;
         hit.dpdu = normalize(transform_normal(( dv2 * dp1 - dv1 * dp2) * invdet, minv));
         hit.dpdv = normalize(transform_normal((-du2 * dp1 + du1 * dp2) * invdet, minv));
     }
     else
     {
         hit.dpdu = orthovector(hit.n);
         hit.dpdv = cross(hit.n, hit.dpdu);
     }
     
     hit.uv = (1.f - a - b) * t1 + a * t2 + b * t3;
     hit.m = face.m;
     hit.bundle = this;
}

void Mesh::FillSample(Face const& face, float a, float b, Sample& sample) const
{
    // Get transform
    matrix m, minv;
    GetTransform(m, minv);
    
    float3 const& p1(vertices_[face.vi0]);
    float3 const& p2(vertices_[face.vi1]);
    float3 const& p3(vertices_[face.vi2]);
    
    float3 const& n1(normals_[face.ni0]);
    float3 const& n2(normals_[face.ni1]);
    float3 const& n3(normals_[face.ni2]);
    
    float2 const& t1(uvs_[face.ti0]);
    float2 const& t2(uvs_[face.ti1]);
    float2 const& t3(uvs_[face.ti2]);
    
    sample.p = transform_point((1.f - a - b) * p1 + a * p2 + b * p3, m);
    sample.n = normalize(transform_normal((1.f - a - b) * n1 + a * n2 + b * n3, minv));
    sample.uv = (1.f - a - b) * t1 + a * t2 + b * t3;
    sample.m = face.m;
    sample.pdf = 1.f / (sqrtf(fabs(cross(p3 - p1, p3 - p2).sqnorm())) * 0.5f);
    assert(!isinf(sample.pdf));
}


Mesh::Mesh(float const* vertices, int vnum, int vstride,
           float const* normals, int nnum, int nstride,
           float const* uvs, int unum, int ustride,
           int const* vidx, int vistride,
           int const* nidx, int nistride,
           int const* uidx, int uistride,
           int const* materials, int mstride,
           int nfaces)
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
    faces_.resize(nfaces);
    for (int i = 0; i < nfaces; ++i)
    {
        faces_[i].vi0 = *((int const*)((char*)vidx + 3 * i * vistride));
        faces_[i].vi1 = *((int const*)((char*)vidx + (3 * i + 1) * vistride));
        faces_[i].vi2 = *((int const*)((char*)vidx + (3 * i + 2) * vistride));

        faces_[i].ni0 = *((int const*)((char*)nidx + 3 * i * nistride));
        faces_[i].ni1 = *((int const*)((char*)nidx + (3 * i + 1) * nistride));
        faces_[i].ni2 = *((int const*)((char*)nidx + (3 * i + 2) * nistride));

        if (hasuv)
        {
            faces_[i].ti0 = *((int const*)((char*)uidx + 3 * i * uistride));
            faces_[i].ti1 = *((int const*)((char*)uidx + (3 * i + 1) * uistride));
            faces_[i].ti2 = *((int const*)((char*)uidx + (3 * i + 2) * uistride));
        }
        else
        {
            faces_[i].ti0 = 0;
            faces_[i].ti1 = 0;
            faces_[i].ti2 = 0;
        }

        faces_[i].m = *((int const*)((char*)materials + i * mstride));
    }
}

bool Mesh::IntersectShape(std::size_t idx, ray const& r, Hit& hit) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    // Get the face
    Face const& face(faces_[idx]);
    
    // Get transform
    matrix m, minv;
    GetTransform(m, minv);
    
    // Transform ray appropriately to object space
    ray ro = transform_ray(r, minv);
    
    float t, a, b;
    if (IntersectFace(face, ro, hit.t, t, a, b))
    {
        FillHit(face, t, a, b, hit);
        return true;
    }
    
    return false;
}

bool Mesh::IntersectShape(std::size_t idx, ray const& r) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    // Get the face
    Face const& face(faces_[idx]);
    
    // Get transform
    matrix m, minv;
    GetTransform(m, minv);
    
    // Transform ray appropriately to object space
    ray ro = transform_ray(r, minv);
    
    float t, a, b;
    if (IntersectFace(face, ro, r.t.y, t, a, b))
    {
        return true;
    }
    
    return false;
}

bbox Mesh::GetShapeWorldBounds(std::size_t idx) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    // Get the face
    Face const& face(faces_[idx]);
    
    // Get transform
    matrix m, minv;
    GetTransform(m, minv);
    
    float3 p1(transform_point(vertices_[face.vi0], m));
    float3 p2(transform_point(vertices_[face.vi1], m));
    float3 p3(transform_point(vertices_[face.vi2], m));
    
    // Calculate box
    bbox bound(p1, p2);
    bound.grow(p3);
    
    return bound;
}

bbox Mesh::GetShapeObjectBounds(std::size_t idx) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    // Get the face
    Face const& face(faces_[idx]);
    
    // Get object space vertices
    float3 const& p1(vertices_[face.vi0]);
    float3 const& p2(vertices_[face.vi1]);
    float3 const& p3(vertices_[face.vi2]);

    // Calculate box
    bbox bound(p1, p2);
    bound.grow(p3);
    
    return bound;
}

float Mesh::GetShapeSurfaceArea(std::size_t idx) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    // Get the face
    Face const& face(faces_[idx]);
    
    // Get transform
    matrix m, minv;
    GetTransform(m, minv);
    
    // Get transformed(!) vertices and calc area
    float3 p1(transform_point(vertices_[face.vi0], m));
    float3 p2(transform_point(vertices_[face.vi1], m));
    float3 p3(transform_point(vertices_[face.vi2], m));
    
    return sqrtf(fabs(cross(p3 - p1, p3 - p2).sqnorm())) * 0.5f;
}

void Mesh::GetSampleOnShape(std::size_t idx, float2 const& uv, Sample& sample) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    // Get the face
    Face const& face(faces_[idx]);
    
    // Generate barycentrics from random samples
    float a = sqrtf(uv.x)*(1.f - uv.y);
    float b = sqrtf(uv.x)*uv.y;
    
    // Fill sample info
    FillSample(face, a, b, sample);
}

void Mesh::GetSampleOnShape(std::size_t idx, float3 const& p, float2 const& uv, Sample& sample) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    GetSampleOnShape(idx, uv, sample);
    
    if (sample.pdf > 0.f)
    {
        // Construct vector to p
        float3 d = sample.p - p;
        
        // Convert PDF to solid angle
        sample.pdf *= (d.sqnorm() / dot(sample.n, -normalize(d)));
    }
}

float Mesh::GetPdfOnShape(std::size_t idx, float3 const& p, float3 const& w) const
{
    assert(idx >= 0 && idx < GetNumShapes());
    
    // TODO: put this to global settings
    ray r(p, w, float2(0.001f, 100000.f));
    
    Hit hit;
    hit.t = r.t.y;
    
    // Intersect this primitive
    if (IntersectShape(idx, r, hit) && dot(-w, hit.n) > 0.f)
    {
        // Construct direction
        float3 d = p - hit.p;
        
        float v = d.sqnorm() / (dot(normalize(d), hit.n) * GetShapeSurfaceArea(idx));
        assert(!isinf(v));
        // Convert surface area PDF to solid angle PDF
        return d.sqnorm() / (dot(normalize(d), hit.n) * GetShapeSurfaceArea(idx));
    }
    else
    {
        // If the ray doesn't intersect the object set PDF to 0
        return 0.f;
    }
}



