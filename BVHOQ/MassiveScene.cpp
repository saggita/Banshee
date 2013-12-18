#include "MassiveScene.h"

#include "eig3.h"

	
MassiveScene::MassiveScene(std::shared_ptr<Mesh> mesh_ptr)
{
	Mesh::Vertex const* vertexData = mesh_ptr->GetVertexArrayPtr();
	unsigned const* indexData = mesh_ptr->GetIndexArrayPtr();

	for (int i=-3; i < 3; i+=1)
		for (int j=-3; j < 3; j+=1)
			for (int k=0; k < 1; k+=1)
			{
				vector3 offset(4*i,4*j,4*k);

				unsigned baseIdx = vertices_.size();
				unsigned startIdx = indices_.size();

				for (int v = 0; v < mesh_ptr->GetVertexCount(); ++v)
				{
					Mesh::Vertex vtx = vertexData[v];
					vtx.position += offset;
					vertices_.push_back(vtx);
				}

				for (int idx = 0; idx < mesh_ptr->GetIndexCount(); ++idx)
				{
					indices_.push_back(mesh_ptr->GetIndexArrayPtr()[idx] + baseIdx);
				}

				BBox b = mesh_ptr->Bounds();
				b.GetMinPoint() += offset;
				b.GetMaxPoint() += offset;

				MeshDesc md = {b, Sphere(), startIdx, mesh_ptr->GetIndexCount()};
				CalcMeshBSphere(md);

				meshes_.push_back(md);
			}
}

MassiveScene::~MassiveScene()
{

}

std::vector<Mesh::Vertex> const& MassiveScene::GetVertices() const
{
	return	vertices_;
}

std::vector<unsigned int> const& MassiveScene::GetIndices() const
{
	return indices_;
}

std::vector<MassiveScene::MeshDesc> const& MassiveScene::GetMeshes() const
{
	return meshes_;
}

std::shared_ptr<MassiveScene> MassiveScene::CreateFromObj(std::string const& fileName)
{
	auto Mesh = Mesh::CreateFromFile(fileName);
	
	return std::make_shared<MassiveScene>(Mesh);
}

void MassiveScene::CalcMeshBSphere(MeshDesc& md)
{
	/// Calc centroid
	vector3 c = vertices_[indices_[md.startIdx]].position;
	for (int i = 0; i < md.numIndices; ++i)
	{
		c += vertices_[indices_[md.startIdx + i]].position;
	}

	float weight = 1.f / md.numIndices; 
	c *= weight;

	matrix3x3 covMatrix;

	/// Build covariation matrix
	for (int i = 0; i < md.numIndices; ++i)
	{
		covMatrix(0,0) += (vertices_[indices_[md.startIdx + i]].position.x() - c.x()) * (vertices_[indices_[md.startIdx + i]].position.x() - c.x());
		covMatrix(1,1) += (vertices_[indices_[md.startIdx + i]].position.y() - c.y()) * (vertices_[indices_[md.startIdx + i]].position.y() - c.y());
		covMatrix(2,2) += (vertices_[indices_[md.startIdx + i]].position.z() - c.z()) * (vertices_[indices_[md.startIdx + i]].position.z() - c.z());

		covMatrix(0,1) += (vertices_[indices_[md.startIdx + i]].position.x() - c.x()) * (vertices_[indices_[md.startIdx + i]].position.y() - c.y());
		covMatrix(1,0) += (vertices_[indices_[md.startIdx + i]].position.x() - c.x()) * (vertices_[indices_[md.startIdx + i]].position.y() - c.y());

		covMatrix(0,2) += (vertices_[indices_[md.startIdx + i]].position.x() - c.x()) * (vertices_[indices_[md.startIdx + i]].position.z() - c.z());
		covMatrix(2,0) += (vertices_[indices_[md.startIdx + i]].position.x() - c.x()) * (vertices_[indices_[md.startIdx + i]].position.z() - c.z());

		covMatrix(1,2) += (vertices_[indices_[md.startIdx + i]].position.y() - c.y()) * (vertices_[indices_[md.startIdx + i]].position.z() - c.z());
		covMatrix(2,1) += (vertices_[indices_[md.startIdx + i]].position.y() - c.y()) * (vertices_[indices_[md.startIdx + i]].position.z() - c.z());
	}

	covMatrix *= weight;

	matrix3x3 eigMatrix;
	vector3   eigValues;
	EigenDecompose( reinterpret_cast<float(*)[3]>(&covMatrix(0,0)), reinterpret_cast<float(*)[3]>(&eigMatrix(0,0)), reinterpret_cast<float*>(&eigValues[0])); 

#define EIGEN_TEST_PRECISION 0.1
#ifdef EIGEN_TEST
	for (int i = 0; i < 3; ++i)
	{
		vector3 evec = vector3(eigMatrix(0,i), eigMatrix(1,i), eigMatrix(2,i));
		vector3 lv = eigValues[i] * evec;
		vector3 testval = covMatrix * evec;
		assert(abs(testval.x()-lv.x()) < EIGEN_TEST_PRECISION || 
			   abs(testval.y()-lv.y()) < EIGEN_TEST_PRECISION || 
			   abs(testval.z()-lv.z()) < EIGEN_TEST_PRECISION);
	}
#endif

	/// Get principal component
	vector3 principalAxe;

	if (eigValues[0] >= eigValues[1] && eigValues[0] >= eigValues[1])
	{
		principalAxe = vector3(eigMatrix(0,0), eigMatrix(1,0), eigMatrix(2,0));
	}

	if (eigValues[1] >= eigValues[0] && eigValues[1] >= eigValues[2])
	{
		principalAxe = vector3(eigMatrix(0,1), eigMatrix(1,1), eigMatrix(2,1));
	}

	if (eigValues[2] >= eigValues[0] && eigValues[2] >= eigValues[1])
	{
		principalAxe = vector3(eigMatrix(0,2), eigMatrix(1,2), eigMatrix(2,2));
	}

	principalAxe.normalize();

	/// Choose max extent along principal axis
	float min, max;
	vector3 vmin, vmax;
	min = max = dot(principalAxe, vertices_[indices_[md.startIdx]].position);
	vmin = vmax = vertices_[indices_[md.startIdx]].position;

	for (int i = 0; i < md.numIndices; ++i)
	{
		float extent = dot(principalAxe, vertices_[indices_[md.startIdx + i]].position);

		if ( extent > max)
		{
			max = extent;
			vmax = vertices_[indices_[md.startIdx + i]].position;
		}
		
		if ( extent < min)
		{
			min = extent;
			vmin = vertices_[indices_[md.startIdx + i]].position;
		}
	}

	/// Approximate bounding Sphere
	vector3 center = 0.5f * (vmin + vmax);
	float   sqRadius = (center - vmin).sq_norm();
	float   radius = sqrt(sqRadius);

	/// Refine Sphere
	for (int i = 0; i < md.numIndices; ++i)
	{
		vector3 point = vertices_[indices_[md.startIdx]].position;
		vector3 cp = center - point;
		if (cp.sq_norm() > sqRadius)
		{
			/// Adjust center and radius to encompass point
			cp.normalize();
			vector3 g = center - radius * cp;
			center = 0.5f * (g + point);
			sqRadius = (center - point).sq_norm();
			radius = sqrt(sqRadius);
		}
	}

#define BSphere_TEST
#ifdef BSphere_TEST
	float dev = sqRadius;
	for (int i = 0; i < md.numIndices; ++i)
	{
		vector3 point = vertices_[indices_[md.startIdx]].position;
		float dist_sq = (point - center).sq_norm();
		assert(dist_sq <= sqRadius);
		dev = std::min(dev, sqRadius - dist_sq);
	}
#endif

	md.bSphere.center = center;
	md.bSphere.radius = radius;
}

