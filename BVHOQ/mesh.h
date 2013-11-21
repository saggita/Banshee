#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <memory>
#include <iosfwd>

#include "common_types.h"
#include "bbox.h"

class mesh
{
public:
    mesh();
	~mesh();

	struct vertex
	{
		vector3 position;
		vector2 texcoord;
		vector3 normal;
	};

	static std::shared_ptr<mesh>  create_from_file(std::string const& file_name);

	vertex const* get_vertex_array_pointer() const;
	unsigned const* get_index_array_pointer() const;
	uint get_vertex_count() const;
	uint get_index_count() const;
	uint get_vertex_size_in_bytes() const;
	bbox bounds() const;

private:
	void load_from_file(std::string const& file_name);

	mesh(mesh const&);
	mesh& operator = (mesh const&);

	std::vector<vertex> interleaved_data_;
	std::vector<unsigned> interleaved_indices_;
	
	bbox bbox_;
};

#endif 