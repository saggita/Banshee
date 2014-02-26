//
//  TestScene.h
//  Launcher
//
//  Created by dmitryk on 02.02.14.
//
//

#ifndef TESTSCENE_H
#define TESTSCENE_H

#include <memory>
#include <vector>
#include <string>

#include "FireRays.h"

class TestScene : public SceneBase
{
public:
    static std::shared_ptr<TestScene> Create();

    TestScene();
    ~TestScene();

    Vertex const*       GetVertices() const;
    unsigned int        GetVertexCount() const;
    unsigned int const* GetIndices() const;
    unsigned int        GetIndexCount() const;
    unsigned int const* GetMaterials() const;
    
    unsigned int        GetMaterialRepCount() const;
    MaterialRep const*  GetMaterialReps() const;

private:
    void BuildMaterials();
    
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<unsigned int> materials_;
    std::vector<MaterialRep>  materialReps_;
};


#endif // TESTSCENE_H