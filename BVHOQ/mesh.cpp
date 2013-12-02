//
//  Mesh.cpp
//  test
//
//  Created by Dmitry Kozlov on 08.04.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <string>
#include <iterator>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace Assimp;

// get rid of crazy ms "switch to non-portable sscanf_s" warning
#pragma warning(push)
#pragma warning(disable:4996)

using namespace std;


mesh::mesh()
{
}

mesh::~mesh()
{
}

void mesh::load_from_file(std::string const& file_name)
{
	Importer importer;
	
	const aiScene* scene = importer.ReadFile(
											 file_name,
											 aiProcess_Triangulate			|
											 aiProcess_JoinIdenticalVertices  |
											 aiProcess_SortByPType | 
											 aiProcess_GenNormals
											 );
	
	if( !scene)
	{
		throw runtime_error("Shit happens: check the path to your models");
	}

	unsigned int base_idx = 0;
	
	bbox_ = bbox(vector3(scene->mMeshes[0]->mVertices[0].x, scene->mMeshes[0]->mVertices[0].y, scene->mMeshes[0]->mVertices[0].z));

	for (int m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[0];
	
		for (int i = 0; i < mesh->mNumFaces; ++i)
		{
			interleaved_indices_.push_back(base_idx + mesh->mFaces[i].mIndices[0]);
			interleaved_indices_.push_back(base_idx + mesh->mFaces[i].mIndices[1]);
			interleaved_indices_.push_back(base_idx + mesh->mFaces[i].mIndices[2]);
		}
	
		for (int i = 0; i < mesh->mNumVertices; ++i)
		{
			vertex v;
			v.position.x() = mesh->mVertices[i].x;
			v.position.y() = mesh->mVertices[i].y;
			v.position.z() = mesh->mVertices[i].z;
		
			if (mesh->HasNormals())
			{
				v.normal.x() = mesh->mNormals[i].x;
				v.normal.y() = mesh->mNormals[i].y;
				v.normal.z() = mesh->mNormals[i].z;
			}
		
			if (mesh->HasTextureCoords(0))
			{
				v.texcoord.x() = mesh->mTextureCoords[0][i].x;
				v.texcoord.y() = mesh->mTextureCoords[0][i].y;
			}
		
			interleaved_data_.push_back(v);
			bbox_ = bbox_union(bbox_, v.position);
		}
		
		base_idx = (unsigned int)interleaved_data_.size();
	}

	std::cout << "Scene " << file_name << " loaded.\n";
	std::cout << interleaved_indices_.size()/3 << " triangles " << 
	interleaved_data_.size() <<" vertices\n";
}

shared_ptr<mesh>  mesh::create_from_file(string const& file_name)
{
	shared_ptr<mesh> res = make_shared<mesh>();
	res->load_from_file(file_name);
	return res;
}

mesh::vertex const* mesh::get_vertex_array_pointer() const
{
	assert(interleaved_data_.size() > 0);
	return &interleaved_data_[0];
}

unsigned const* mesh::get_index_array_pointer() const
{
	assert(interleaved_indices_.size() > 0);
	return &interleaved_indices_[0];
}

uint mesh::get_vertex_count() const
{
	assert(interleaved_data_.size() > 0);
	return static_cast<unsigned>(interleaved_data_.size());
}

uint mesh::get_index_count() const
{
	assert(interleaved_indices_.size() > 0);
	return static_cast<unsigned>(interleaved_indices_.size());
}

uint mesh::get_vertex_size_in_bytes() const
{
	return sizeof(vertex);
}

bbox mesh::bounds() const
{
	return bbox_;
}


#pragma warning(pop)