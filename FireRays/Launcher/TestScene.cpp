//
//  TestScene.cpp
//  Launcher
//
//  Created by dmitryk on 02.02.14.
//
//

#include "TestScene.h"

#include "Mesh.h"
#include "eig3.h"

static SceneBase::Vertex MeshVertexToSceneVertex(Mesh::Vertex const& v)
{
    SceneBase::Vertex vtx;
    vtx.position = v.position;
    vtx.texcoord = v.texcoord;
    vtx.normal   = v.normal;
    return vtx;
}

std::shared_ptr<TestScene> TestScene::Create()
{
    return std::make_shared<TestScene>();
}

TestScene::TestScene()
{
    BuildMaterials();
    
    // Add monkey w/ glossy material
    {
        std::shared_ptr<Mesh> mesh_ptr = Mesh::CreateFromFile("../../../Resources/sphere.objm");
        mesh_ptr->Rescale(2.5f);
        Mesh::Vertex const* vertexData = mesh_ptr->GetVertexArrayPtr();
        unsigned const* indexData = mesh_ptr->GetIndexArrayPtr();


        for (int v = 0; v < mesh_ptr->GetVertexCount(); ++v)
        {
            vertices_.push_back(MeshVertexToSceneVertex(vertexData[v]));
        }

        for (int idx = 0; idx < mesh_ptr->GetIndexCount(); ++idx)
        {
            indices_.push_back(indexData[idx]);
        }

        unsigned material = 0;
        for (int idx = 0; idx < mesh_ptr->GetIndexCount()/3; ++idx)
            materials_.push_back(material);
    }

    unsigned startIdx = vertices_.size();

    // Add monkey w/ diffuse material
    {
        std::shared_ptr<Mesh> mesh_ptr = Mesh::CreateFromFile("../../../Resources/sphere.objm");
        mesh_ptr->Rescale(2.5f);
        Mesh::Vertex const* vertexData = mesh_ptr->GetVertexArrayPtr();
        unsigned const* indexData = mesh_ptr->GetIndexArrayPtr();


        for (int v = 0; v < mesh_ptr->GetVertexCount(); ++v)
        {
            vertices_.push_back(MeshVertexToSceneVertex(vertexData[v]));
            vertices_[vertices_.size() - 1].position += vector3(6, 0, 0);
        }

        for (int idx = 0; idx < mesh_ptr->GetIndexCount(); ++idx)
        {
            indices_.push_back(startIdx + indexData[idx]);
        }

        unsigned material = 1;
        for (int idx = 0; idx < mesh_ptr->GetIndexCount()/3; ++idx)
            materials_.push_back(material);
    }


    startIdx = vertices_.size();

    // Add plane w/ diffuse material
    Vertex v;

    v.position = vector3(-500, -3, -500);
    v.normal   = vector3(0, 1, 0);
    v.texcoord = vector2(0, 0);

    vertices_.push_back(v);

    v.position = vector3(500, -3, -500);
    v.normal   = vector3(0, 1, 0);
    v.texcoord = vector2(25, 0);

    vertices_.push_back(v);

    v.position = vector3(500, -3,  500);
    v.normal   = vector3(0, 1, 0);
    v.texcoord = vector2(25, 25);

    vertices_.push_back(v);

    v.position = vector3(-500, -3,  500);
    v.normal   = vector3(0, 1, 0);
    v.texcoord = vector2(0, 25);

    vertices_.push_back(v);

    indices_.push_back(startIdx);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 2);

    materials_.push_back(3);
    materials_.push_back(3);

    startIdx = vertices_.size();

    v.position = vector3(2, 7, -1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(0, 0);

    vertices_.push_back(v);

    v.position = vector3(4, 7, -1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(1, 0);

    vertices_.push_back(v);

    v.position = vector3(4, 7,  1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(1, 1);

    vertices_.push_back(v);

    v.position = vector3(2, 7,  1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(0, 1);

    vertices_.push_back(v);

    indices_.push_back(startIdx);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 2);

    materials_.push_back(2);
    materials_.push_back(2);

    startIdx = vertices_.size();


    v.position = vector3(-20, -3, -5);
    v.normal   = vector3(0, 0, 1);
    v.texcoord = vector2(0, 0);

    vertices_.push_back(v);

    v.position = vector3(20, -3, -5);
    v.normal   = vector3(0, 0, 1);
    v.texcoord = vector2(1, 0);

    vertices_.push_back(v);

    v.position = vector3(20, 3,  -5);
    v.normal   = vector3(0, 0, 1);
    v.texcoord = vector2(1, 1);

    vertices_.push_back(v);

    v.position = vector3(-20, 3, -5);
    v.normal   = vector3(0, 0, 1);
    v.texcoord = vector2(0, 1);

    vertices_.push_back(v);

    indices_.push_back(startIdx);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 2);

    materials_.push_back(4);
    materials_.push_back(4);
}

TestScene::~TestScene()
{
}

TestScene::Vertex const*       TestScene::GetVertices() const
{
    return &vertices_[0];
}
unsigned int        TestScene::GetVertexCount() const
{
    return (unsigned int)vertices_.size();
}

unsigned int const* TestScene::GetIndices() const
{
    return &indices_[0];
}

unsigned int        TestScene::GetIndexCount() const
{
    return (unsigned int)indices_.size();
}

unsigned int const* TestScene::GetMaterials() const
{
    return &materials_[0];
}

void TestScene::BuildMaterials()
{
    MaterialRep materialRep;
    
    materialRep.eBsdf = 2;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 0;
    materialRep.vKd.x() = materialRep.vKd.y() = materialRep.vKd.z() = materialRep.vKd.w() = 0;
    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 1.0;
    materialRep.fEs = 1280.f;
    materialRep.uTd = -1;
    materialReps_.push_back(materialRep);
    
    materialRep.eBsdf = 1;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 0;
    materialRep.vKd.x() = 0.6f;
    materialRep.vKd.y() = 0.1f;
    materialRep.vKd.z() = materialRep.vKd.w() = 0.1f;
    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 0.0;
    materialRep.fEs = 0.f;
    materialRep.uTd = -1;
    materialReps_.push_back(materialRep);
    
    materialRep.eBsdf = 3;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 20.0f;
    materialRep.vKd.x() = materialRep.vKd.y() = materialRep.vKd.z() = materialRep.vKd.w() = 0.0;
    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 0.0;
    materialRep.fEs = 0.f;
    materialRep.uTd = -1;
    materialReps_.push_back(materialRep);
    
    materialRep.eBsdf = 1;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 0;
    materialRep.vKd.x() = materialRep.vKd.y() = materialRep.vKd.z() = materialRep.vKd.w() = 0.f;
    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 0.0;
    materialRep.fEs = 0.f;
    materialRep.uTd = 0;
    materialReps_.push_back(materialRep);

    materialRep.eBsdf = 1;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 0;

    materialRep.vKd.x() = 0.2f;
    materialRep.vKd.y() = 0.9f;
    materialRep.vKd.z() = 0.2f;
    materialRep.vKd.w() = 0.0;

    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 0.0;
    materialRep.fEs = 0.f;
    materialRep.uTd = -1;
    materialReps_.push_back(materialRep);
}

unsigned int TestScene::GetMaterialRepCount() const
{
    return (unsigned int)materialReps_.size();
    
    
}

TestScene::MaterialRep const*  TestScene::GetMaterialReps() const
{
    return &materialReps_[0];
}


