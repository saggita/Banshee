#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <memory>
#include <iosfwd>

#include "CommonTypes.h"
#include "BBox.h"

class Mesh
{
public:
	Mesh();
	~Mesh();

	struct Vertex
	{
		vector3 position;
		vector2 texcoord;
		vector3 normal;
	};

	static std::shared_ptr<Mesh>  CreateFromFile(std::string const& fileName);

	Vertex const* GetVertexArrayPtr() const;
	unsigned const* GetIndexArrayPtr() const;
	uint GetVertexCount() const;
	uint GetIndexCount() const;
	uint GetVertexSizeInBytes() const;
	BBox Bounds() const;

private:
	void LoadFromFile(std::string const& fileName);

	Mesh(Mesh const&);
	Mesh& operator = (Mesh const&);

	std::vector<Vertex> vertexData_;
	std::vector<unsigned> indexData_;
	
	BBox bBox_;
};

#endif 