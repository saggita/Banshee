#include "massive_scene.h"

    
massive_scene::massive_scene(std::shared_ptr<mesh> mesh_ptr)
{
	mesh::vertex const* vertex_data = mesh_ptr->get_vertex_array_pointer();
    unsigned const* index_data = mesh_ptr->get_index_array_pointer();

	for (int i=-5; i < 5; i+=1)
		for (int j=-5; j < 5; j+=1)
			for (int k=0; k < 3; k+=1)
			{
				vector3 offset(2*i,2*j,2*k);

				unsigned base_idx = vertices_.size();

				for (int v = 0; v < mesh_ptr->get_vertex_count(); ++v)
				{
					vertices_.push_back(vertex_data[v].position + offset);
				}

				for (int idx = 0; idx < mesh_ptr->get_index_count(); ++idx)
				{
					indices_.push_back(mesh_ptr->get_index_array_pointer()[idx] + base_idx);
				}
			}
}

massive_scene::~massive_scene()
{

}

std::vector<vector3> const& massive_scene::vertices() const
{
	return	vertices_;
}

std::vector<unsigned int> const& massive_scene::indices() const
{
	return indices_;
}

std::shared_ptr<massive_scene> massive_scene::create_from_obj(std::string const& file_name)
{
	auto mesh = mesh::create_from_file(file_name);
    
    return std::make_shared<massive_scene>(mesh);
}

