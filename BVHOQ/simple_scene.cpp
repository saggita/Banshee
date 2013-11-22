//
//  simple_scene.cpp
//  BVHOQ
//
//  Created by dmitryk on 08.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "simple_scene.h"

std::shared_ptr<simple_scene> simple_scene::create_from_obj(std::string const& file_name)
{
	auto mesh = mesh::create_from_file(file_name);
	
	return std::make_shared<simple_scene>(mesh);
}

std::vector<mesh::vertex> const& simple_scene::vertices() const
{
	return vertices_;
}

std::vector<unsigned int> const& simple_scene::indices() const
{
	return indices_;
}

simple_scene::~simple_scene()
{
}

simple_scene::simple_scene(std::shared_ptr<mesh> mesh_ptr)
{
	mesh::vertex const* vertex_data = mesh_ptr->get_vertex_array_pointer();
	unsigned const* index_data = mesh_ptr->get_index_array_pointer();
	vertices_.resize(mesh_ptr->get_vertex_count());
	indices_.resize(mesh_ptr->get_index_count());
	
	bbox b = bbox(vertex_data[0].position);
	for (int i = 0; i < mesh_ptr->get_vertex_count(); ++i)
	{
		vertices_[i] = vertex_data[i];
		b = bbox_union(b, vertex_data[i].position);
	}
	std::copy(index_data, index_data + mesh_ptr->get_index_count(), indices_.begin());

	mesh_desc md = {b, 0, indices_.size()};
	meshes_.push_back(md);
}

std::vector<simple_scene::mesh_desc> const& simple_scene::meshes() const
{
	return meshes_;
}
