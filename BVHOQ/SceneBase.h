//
//  SceneBase.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_SceneBase_h
#define BVHOQ_SceneBase_h

#include <vector>
#include "CommonTypes.h"
#include "BBox.h"
#include "Mesh.h"

class SceneBase
{
public:

	struct Sphere
	{
		vector3 center;
		float   radius;
	};

	struct MeshDesc 
	{
		BBox box;
		Sphere bSphere;
		unsigned startIdx;
		unsigned numIndices;
	};

	virtual ~SceneBase() = 0;
	virtual std::vector<Mesh::Vertex> const& GetVertices() const = 0;
	virtual std::vector<unsigned int> const& GetIndices() const = 0;
	virtual std::vector<MeshDesc> const& GetMeshes() const = 0;
	
private:
	SceneBase& operator= (SceneBase const&);
};

inline SceneBase::~SceneBase(){}



#endif
