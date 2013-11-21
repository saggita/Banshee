//
//  simple_scene.h
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

#include "scene_base.h"
#include "mesh.h"
#include "bbox.h"


class massive_scene : public scene_base
{
public:
    static std::shared_ptr<massive_scene> create_from_obj(std::string const& file_name);
    
    massive_scene(std::shared_ptr<mesh> mesh_ptr);
    ~massive_scene();
    
    std::vector<vector3> const& vertices() const;
    std::vector<unsigned int> const& indices() const;
	std::vector<bbox> const& bounds() const;
    
    
private:
    std::vector<vector3> vertices_;
    std::vector<unsigned int> indices_;
	std::vector<bbox> bboxes_;
};



#endif
