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
	
	std::vector<Vertex> const& GetVertices() const;
	std::vector<unsigned int> const& GetIndices() const;
    std::vector<unsigned int> const& GetMaterials() const;
	
private:
    
	std::vector<Vertex> vertices_;
	std::vector<unsigned int> indices_;
    std::vector<unsigned int> materials_;
};


#endif // TESTSCENE_H