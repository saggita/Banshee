//
//  ObjScene.cpp
//  Launcher
//
//  Created by dmitryk on 11.05.14.
//
//

#include "ObjScene.h"

#include "utils.h"

#include "Importer.hpp"
#include "Mesh.h"
#include "postprocess.h"
#include "scene.h"

using namespace Assimp;
using namespace std;

// get rid of crazy ms "switch to non-portable sscanf_s" warning
#pragma warning(push)
#pragma warning(disable:4996)

std::shared_ptr<ObjScene> ObjScene::Create(std::string const& fileName)
{
    return std::shared_ptr<ObjScene>(new ObjScene(fileName));
}


ObjScene::ObjScene(std::string const& fileName)
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
		const aiMesh* mesh = scene->mMeshes[m];
        
		for (int i = 0; i < mesh->mNumFaces; ++i)
		{
			indices_.push_back(baseIdx + mesh->mFaces[i].mIndices[0]);
			indices_.push_back(baseIdx + mesh->mFaces[i].mIndices[1]);
			indices_.push_back(baseIdx + mesh->mFaces[i].mIndices[2]);
            
            materials_.push_back(mesh->mMaterialIndex);
		}
        
		for (int i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex v;
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
            
			vertices_.push_back(v);
		}
        
        
		baseIdx = (unsigned int)vertices_.size();
	}
    
    BuildMaterials(scene);
}

ObjScene::~ObjScene()
{
    
}

ObjScene::Vertex const*       ObjScene::GetVertices() const
{
    return &vertices_[0];
}
unsigned int        ObjScene::GetVertexCount() const
{
    return (unsigned int)vertices_.size();
}

unsigned int const* ObjScene::GetIndices() const
{
    return &indices_[0];
}

unsigned int        ObjScene::GetIndexCount() const
{
    return (unsigned int)indices_.size();
}

unsigned int const* ObjScene::GetMaterials() const
{
    return &materials_[0];
}

void ObjScene::BuildMaterials(aiScene const* scene)
{

    
    for (int i = 0; i < scene->mNumMaterials; ++i)
    {
        MaterialRep materialRep;

        aiMaterial* material = scene->mMaterials[i];
        
        aiColor3D diffuse(0,0,0);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        
        aiColor3D specular(0,0,0);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        
        aiColor3D emissive(0,0,0);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        
        if (name == aiString("light"))
        {
            materialRep.vKe.x() = 100;//emissive.r;
            materialRep.vKe.y() = 100;//emissive.g;
            materialRep.vKe.z() = 75;//emissive.b;
            materialRep.uTd = -1;
            materialRep.eBsdf = 3;
        }
        else if(name == aiString("sphere"))
        {
            materialRep.vKs.x() = 1;//emissive.r;
            materialRep.vKs.y() = 1;//emissive.g;
            materialRep.vKs.z() = 1;//emissive.b;
            materialRep.uTd = -1;
            materialRep.eBsdf = 2;
            materialRep.fEs = 4000.f;
        }
        else
        {
            materialRep.vKd.x() = diffuse.r;
            materialRep.vKd.y() = diffuse.g;
            materialRep.vKd.z() = diffuse.b;
            materialRep.uTd = -1;
            materialRep.eBsdf = 1;
        }
        
        materialReps_.push_back(materialRep);
    }
}

unsigned int ObjScene::GetMaterialRepCount() const
{
    return (unsigned int)materialReps_.size();
}

ObjScene::MaterialRep const*  ObjScene::GetMaterialReps() const
{
    return &materialReps_[0];
}


