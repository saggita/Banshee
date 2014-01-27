//
//  SimpleScene.h
//  BVHOQ
//
//  Created by dmitryk on 08.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_massive_scene_h
#define BVHOQ_massive_scene_h

#include <memory>
#include <vector>
#include <string>

#include "FireRays.h"

class MassiveScene : public SceneBase
{
public:
	static std::shared_ptr<MassiveScene> Create();
	
	MassiveScene();
	~MassiveScene();
	
	std::vector<Vertex> const& GetVertices() const;
	std::vector<unsigned int> const& GetIndices() const;
    std::vector<unsigned int> const& GetMaterials() const;
	
private:

	std::vector<Vertex> vertices_;
	std::vector<unsigned int> indices_;
    std::vector<unsigned int> materials_;
};



#endif
