//
//  Mesh.cpp
//  test
//
//  Created by Dmitry Kozlov on 08.04.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <string>
#include <iterator>
#include <map>

#include "Importer.hpp"
#include "Mesh.h"
#include "postprocess.h"
#include "scene.h"

using namespace Assimp;

// get rid of crazy ms "switch to non-portable sscanf_s" warning
#pragma warning(push)
#pragma warning(disable:4996)

using namespace std;


Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::LoadFromFile(std::string const& fileName)
{
	Importer importer;
	
	const aiScene* scene = importer.ReadFile(
											 fileName,
											 aiProcess_Triangulate			|
											 aiProcess_JoinIdenticalVertices  |
											 aiProcess_SortByPType | 
											 aiProcess_GenNormals
											 );
	
	if( !scene)
	{
		throw runtime_error("Shit happens: check the path to your models");
	}

	unsigned int baseIdx = 0;

	for (int m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* Mesh = scene->mMeshes[m];
	
		for (int i = 0; i < Mesh->mNumFaces; ++i)
		{
			indexData_.push_back(baseIdx + Mesh->mFaces[i].mIndices[0]);
			indexData_.push_back(baseIdx + Mesh->mFaces[i].mIndices[1]);
			indexData_.push_back(baseIdx + Mesh->mFaces[i].mIndices[2]);
		}
	
		for (int i = 0; i < Mesh->mNumVertices; ++i)
		{
			Vertex v;
			v.position.x() = Mesh->mVertices[i].x;
			v.position.y() = Mesh->mVertices[i].y;
			v.position.z() = Mesh->mVertices[i].z;
		
			if (Mesh->HasNormals())
			{
				v.normal.x() = Mesh->mNormals[i].x;
				v.normal.y() = Mesh->mNormals[i].y;
				v.normal.z() = Mesh->mNormals[i].z;
			}
		
			if (Mesh->HasTextureCoords(0))
			{
				v.texcoord.x() = Mesh->mTextureCoords[0][i].x;
				v.texcoord.y() = Mesh->mTextureCoords[0][i].y;
			}
		
			vertexData_.push_back(v);
		}
		
		baseIdx = (unsigned int)vertexData_.size();
	}

	std::cout << "Scene " << fileName << " loaded.\n";
	std::cout << indexData_.size()/3 << " triangles " << 
	vertexData_.size() <<" vertices\n";
}

shared_ptr<Mesh>  Mesh::CreateFromFile(string const& fileName)
{
	shared_ptr<Mesh> res = make_shared<Mesh>();
	res->LoadFromFile(fileName);
	return res;
}

Mesh::Vertex const* Mesh::GetVertexArrayPtr() const
{
	assert(vertexData_.size() > 0);
	return &vertexData_[0];
}

unsigned const* Mesh::GetIndexArrayPtr() const
{
	assert(indexData_.size() > 0);
	return &indexData_[0];
}

uint Mesh::GetVertexCount() const
{
	assert(vertexData_.size() > 0);
	return static_cast<unsigned>(vertexData_.size());
}

uint Mesh::GetIndexCount() const
{
	assert(indexData_.size() > 0);
	return static_cast<unsigned>(indexData_.size());
}

uint Mesh::GetVertexSizeInBytes() const
{
	return sizeof(Vertex);
}

void Mesh::Rescale(float factor)
{
    vector3 c(0,0,0);
    
    std::for_each(vertexData_.begin(), vertexData_.end(), [&c](Mesh::Vertex const& v){ c += v.position; });
    
    c *= 1.f / vertexData_.size();
    
    std::for_each(vertexData_.begin(), vertexData_.end(),
        [this, c, factor](Mesh::Vertex& v)
                  {
                      vector3 c2v = factor * (v.position - c);
                      v.position = c + c2v;
                  });
}


#pragma warning(pop)