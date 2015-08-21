#include "embree.h"
#include "../primitive/mesh.h"
#include "../math/mathutils.h"

#include <map>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

struct Embree::EmbreeData
{
    RTCScene scene;
    
    std::map<int, Mesh*> geom2mesh;
};

Embree::Embree()
: embreedata_(new EmbreeData)
{
    rtcInit();
    
    embreedata_->scene = rtcNewScene(RTC_SCENE_STATIC, RTC_INTERSECT1);
}

Embree::~Embree()
{
    rtcDeleteScene(embreedata_->scene);
    rtcExit();
}

// Build function: pass bounding boxes and
void Embree::Build(std::vector<std::unique_ptr<ShapeBundle>> const& bundles)
{
    for (size_t i = 0; i < bundles.size(); ++i)
    {
        Mesh* mesh = dynamic_cast<Mesh*>(bundles[i].get());
        
        if (!mesh)
        {
            throw std::runtime_error("Embree: Non-supported primitive type");
        }
        
        int geomid = rtcNewTriangleMesh(embreedata_->scene, RTC_GEOMETRY_STATIC, mesh->GetNumFaces(), mesh->GetNumVertices());
        
        embreedata_->geom2mesh[geomid] = mesh;
        
        struct Vertex   { float x, y, z, a; };
        struct Triangle { int v0, v1, v2; };
        
        Vertex* vertices = (Vertex*) rtcMapBuffer(embreedata_->scene, geomid, RTC_VERTEX_BUFFER);

        matrix m, minv;
        mesh->GetTransform(m, minv);
        
        auto vertexdata = mesh->GetVertices();
        for (size_t j = 0; j < mesh->GetNumVertices(); ++j)
        {
            float3 v = transform_point(vertexdata[j], m);
            vertices[j].x = v.x;//vertexdata[j].x;
            vertices[j].y = v.y;//vertexdata[j].y;
            vertices[j].z = v.z;//vertexdata[j].z;
            vertices[j].a = 1.f;
        }
        
        rtcUnmapBuffer(embreedata_->scene, geomid, RTC_VERTEX_BUFFER);
        
        Triangle* triangles = (Triangle*)rtcMapBuffer(embreedata_->scene, geomid, RTC_INDEX_BUFFER);
        
        auto faces = mesh->GetFaces();
        for (size_t j = 0; j < mesh->GetNumFaces(); ++j)
        {
            triangles[j].v0 = faces[j].vi0;
            triangles[j].v1 = faces[j].vi1;
            triangles[j].v2 = faces[j].vi2;
        }
        
        rtcUnmapBuffer(embreedata_->scene, geomid, RTC_INDEX_BUFFER);
    }
    
    rtcCommit(embreedata_->scene);
}

/**
 Intersectable overrides
 */
bool Embree::Intersect(ray const& r, ShapeBundle::Hit& hit) const
{
    RTCRay er;
    
    er.org[0] = r.o.x;
    er.org[1] = r.o.y;
    er.org[2] = r.o.z;
    
    er.dir[0] = r.d.x;
    er.dir[1] = r.d.y;
    er.dir[2] = r.d.z;
    
    er.tnear = r.t.x;
    er.tfar = r.t.y;
    er.mask = 0xFFFFFFFF;
    er.time = 0.f;
    
    er.geomID = RTC_INVALID_GEOMETRY_ID;
    er.primID = RTC_INVALID_GEOMETRY_ID;
    er.instID = RTC_INVALID_GEOMETRY_ID;
    
    rtcIntersect(embreedata_->scene, er);
    
    if (er.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        Mesh* mesh = embreedata_->geom2mesh[er.geomID];
        
        mesh->FillHit(er.primID, er.tfar, er.u, er.v, hit);
        
        return true;
    }
    else
    {
        return false;
    }
}

bool Embree::Intersect(ray const& r) const
{
    RTCRay er;
    
    er.org[0] = r.o.x;
    er.org[1] = r.o.y;
    er.org[2] = r.o.z;
    
    er.dir[0] = r.d.x;
    er.dir[1] = r.d.y;
    er.dir[2] = r.d.z;
    
    er.tnear = r.t.x;
    er.tfar = r.t.y;
    er.mask = 0xFFFFFFFF;
    er.time = 0.f;
    
    er.geomID = RTC_INVALID_GEOMETRY_ID;
    er.primID = RTC_INVALID_GEOMETRY_ID;
    er.instID = RTC_INVALID_GEOMETRY_ID;
    
    rtcOccluded(embreedata_->scene, er);
    
    if (er.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        return true;
    }
    else
    {
        return false;
    }
}