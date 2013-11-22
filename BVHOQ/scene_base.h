//
//  scene_base.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_scene_base_h
#define BVHOQ_scene_base_h

#include <vector>
#include "common_types.h"
#include "bbox.h"

class scene_base
{
public:
	struct mesh_desc 
	{
		bbox box;
		unsigned start_idx;
		unsigned num_idx;
	};

	virtual ~scene_base() = 0;
	virtual std::vector<vector3> const& vertices() const = 0;
	virtual std::vector<unsigned int> const& indices() const = 0;
	virtual std::vector<mesh_desc> const& meshes() const = 0;
	
private:
	scene_base& operator= (scene_base const&);
};

inline scene_base::~scene_base(){}



#endif
