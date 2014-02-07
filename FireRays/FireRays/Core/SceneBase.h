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

// SceneBase defines the interface which is 
// used by the RenderBase to access scene data
// such as
// -- geometry
// -- material representations
// -- textures
class SceneBase
{
public:
    // SceneBase vertex type
    struct Vertex;

    // SceneBase public methods
    SceneBase();
    virtual ~SceneBase() = 0;
    virtual Vertex const*       GetVertices() const = 0;
    virtual unsigned int        GetVertexCount() const = 0;
    virtual unsigned int const* GetIndices() const = 0;
    virtual unsigned int        GetIndexCount() const = 0;
    virtual unsigned int const* GetMaterials() const = 0;

private:
    SceneBase(SceneBase const&);
    SceneBase& operator= (SceneBase const&);
};

inline SceneBase::SceneBase(){}
inline SceneBase::~SceneBase(){}


struct SceneBase::Vertex
{
    vector3 position;
    vector2 texcoord;
    vector3 normal;
};

#endif //SCENEBASE_H
