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


TestScene::TestScene()
{
    // Add monkey w/ diffuse material
    {
        std::shared_ptr<Mesh> mesh_ptr = Mesh::CreateFromFile("../../../Resources/sphere.obj");
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
    
    
    materials_.push_back(1);
    materials_.push_back(1);
}

TestScene::~TestScene()
{
    
}

std::vector<SceneBase::Vertex> const& TestScene::GetVertices() const
{
	return	vertices_;
}

std::vector<unsigned int> const& TestScene::GetIndices() const
{
	return indices_;
}

std::shared_ptr<TestScene> TestScene::Create()
{
	return std::make_shared<TestScene>();
}

std::vector<unsigned int> const& TestScene::GetMaterials() const
{
    return materials_;
}


