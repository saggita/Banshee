//
//  SceneBase.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef SCENEBASE_H
#define SCENEBASE_H

#include "Common.h"
#include "CommonTypes.h"

class SceneBase
{
public:

    struct Vertex
    {
        vector3 position;
		vector2 texcoord;
		vector3 normal;
    };

	virtual ~SceneBase() = 0;
	virtual std::vector<Vertex> const& GetVertices() const = 0;
	virtual std::vector<unsigned int> const& GetIndices() const = 0;
    virtual std::vector<unsigned int> const& GetMaterials() const = 0;
	
private:
	SceneBase& operator= (SceneBase const&);
};

inline SceneBase::~SceneBase(){}

#endif //SCENEBASE_H
