//
//  SimpleScene.cpp
//  BVHOQ
//
//  Created by dmitryk on 08.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "SimpleScene.h"
#include "Mesh.h"

#include <memory>

static SceneBase::Vertex MeshVertexToSceneVertex(Mesh::Vertex const& v)
{
    SceneBase::Vertex vtx;
    vtx.position = v.position;
    vtx.texcoord = v.texcoord;
    vtx.normal   = v.normal;
    return vtx;
}

std::shared_ptr<SimpleScene> SimpleScene::CreateFromObj(std::string const& fileName)
{
	auto mesh = Mesh::CreateFromFile(fileName);
	
	return std::make_shared<SimpleScene>(mesh);
}

std::vector<SceneBase::Vertex> const& SimpleScene::GetVertices() const
{
	return vertices_;
}

std::vector<unsigned int> const& SimpleScene::GetIndices() const
{
	return indices_;
}

SimpleScene::~SimpleScene()
{
}

SimpleScene::SimpleScene(std::shared_ptr<Mesh> mesh_ptr)
{
	Mesh::Vertex const* vertexData = mesh_ptr->GetVertexArrayPtr();
	unsigned const* indexData = mesh_ptr->GetIndexArrayPtr();
	vertices_.resize(mesh_ptr->GetVertexCount());
	indices_.resize(mesh_ptr->GetIndexCount());
    materials_.resize(mesh_ptr->GetIndexCount()/3);
	
	for (int i = 0; i < mesh_ptr->GetVertexCount(); ++i)
	{
		vertices_[i] = MeshVertexToSceneVertex(vertexData[i]);
	}

	std::copy(indexData, indexData + mesh_ptr->GetIndexCount(), indices_.begin());
    std::fill(materials_.begin(), materials_.end(), 0);
}

std::vector<unsigned int> const& SimpleScene::GetMaterials() const
{
    return materials_;
}
