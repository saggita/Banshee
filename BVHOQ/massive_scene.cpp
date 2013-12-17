#include "massive_scene.h"

#include "eig3.h"

	
massive_scene::massive_scene(std::shared_ptr<mesh> mesh_ptr)
{
	mesh::vertex const* vertex_data = mesh_ptr->get_vertex_array_pointer();
	unsigned const* index_data = mesh_ptr->get_index_array_pointer();

	for (int i=-3; i < 3; i+=1)
		for (int j=-3; j < 3; j+=1)
			for (int k=0; k < 1; k+=1)
			{
				vector3 offset(4*i,4*j,4*k);

				unsigned base_idx = vertices_.size();
				unsigned start_idx = indices_.size();

				for (int v = 0; v < mesh_ptr->get_vertex_count(); ++v)
				{
					mesh::vertex vtx = vertex_data[v];
					vtx.position += offset;
					vertices_.push_back(vtx);
				}

				for (int idx = 0; idx < mesh_ptr->get_index_count(); ++idx)
				{
					indices_.push_back(mesh_ptr->get_index_array_pointer()[idx] + base_idx);
				}

				bbox b = mesh_ptr->bounds();
				b.min() += offset;
				b.max() += offset;

				mesh_desc md = {b, sphere(), start_idx, mesh_ptr->get_index_count()};
				calc_mesh_bsphere(md);

				meshes_.push_back(md);
			}
}

massive_scene::~massive_scene()
{

}

std::vector<mesh::vertex> const& massive_scene::vertices() const
{
	return	vertices_;
}

std::vector<unsigned int> const& massive_scene::indices() const
{
	return indices_;
}

std::vector<massive_scene::mesh_desc> const& massive_scene::meshes() const
{
	return meshes_;
}

std::shared_ptr<massive_scene> massive_scene::create_from_obj(std::string const& file_name)
{
	auto mesh = mesh::create_from_file(file_name);
	
	return std::make_shared<massive_scene>(mesh);
}

void massive_scene::calc_mesh_bsphere(mesh_desc& md)
{
	/// Calc centroid
	vector3 c = vertices_[indices_[md.start_idx]].position;
	for (int i = 0; i < md.num_idx; ++i)
	{
		c += vertices_[indices_[md.start_idx + i]].position;
	}

	float weight = 1.f / md.num_idx; 
	c *= weight;

	matrix3x3 cov_mat;

	/// Build covariation matrix
	for (int i = 0; i < md.num_idx; ++i)
	{
		cov_mat(0,0) += (vertices_[indices_[md.start_idx + i]].position.x() - c.x()) * (vertices_[indices_[md.start_idx + i]].position.x() - c.x());
		cov_mat(1,1) += (vertices_[indices_[md.start_idx + i]].position.y() - c.y()) * (vertices_[indices_[md.start_idx + i]].position.y() - c.y());
		cov_mat(2,2) += (vertices_[indices_[md.start_idx + i]].position.z() - c.z()) * (vertices_[indices_[md.start_idx + i]].position.z() - c.z());

		cov_mat(0,1) += (vertices_[indices_[md.start_idx + i]].position.x() - c.x()) * (vertices_[indices_[md.start_idx + i]].position.y() - c.y());
		cov_mat(1,0) += (vertices_[indices_[md.start_idx + i]].position.x() - c.x()) * (vertices_[indices_[md.start_idx + i]].position.y() - c.y());

		cov_mat(0,2) += (vertices_[indices_[md.start_idx + i]].position.x() - c.x()) * (vertices_[indices_[md.start_idx + i]].position.z() - c.z());
		cov_mat(2,0) += (vertices_[indices_[md.start_idx + i]].position.x() - c.x()) * (vertices_[indices_[md.start_idx + i]].position.z() - c.z());

		cov_mat(1,2) += (vertices_[indices_[md.start_idx + i]].position.y() - c.y()) * (vertices_[indices_[md.start_idx + i]].position.z() - c.z());
		cov_mat(2,1) += (vertices_[indices_[md.start_idx + i]].position.y() - c.y()) * (vertices_[indices_[md.start_idx + i]].position.z() - c.z());
	}

	cov_mat *= weight;

	matrix3x3 e_mat;
	vector3   e_val;
	eigen_decomposition( reinterpret_cast<float(*)[3]>(&cov_mat(0,0)), reinterpret_cast<float(*)[3]>(&e_mat(0,0)), reinterpret_cast<float*>(&e_val[0])); 

#define EIGEN_TEST_PRECISION 0.1
#ifdef EIGEN_TEST
	for (int i = 0; i < 3; ++i)
	{
		vector3 evec = vector3(e_mat(0,i), e_mat(1,i), e_mat(2,i));
		vector3 lv = e_val[i] * evec;
		vector3 testval = cov_mat * evec;
		assert(abs(testval.x()-lv.x()) < EIGEN_TEST_PRECISION || 
			   abs(testval.y()-lv.y()) < EIGEN_TEST_PRECISION || 
			   abs(testval.z()-lv.z()) < EIGEN_TEST_PRECISION);
	}
#endif

	/// Get principal component
	vector3 principal_axis;

	if (e_val[0] >= e_val[1] && e_val[0] >= e_val[1])
	{
		principal_axis = vector3(e_mat(0,0), e_mat(1,0), e_mat(2,0));
	}

	if (e_val[1] >= e_val[0] && e_val[1] >= e_val[2])
	{
		principal_axis = vector3(e_mat(0,1), e_mat(1,1), e_mat(2,1));
	}

	if (e_val[2] >= e_val[0] && e_val[2] >= e_val[1])
	{
		principal_axis = vector3(e_mat(0,2), e_mat(1,2), e_mat(2,2));
	}

	principal_axis.normalize();

	/// Choose max extent along principal axis
	float min, max;
	vector3 vmin, vmax;
	min = max = dot(principal_axis, vertices_[indices_[md.start_idx]].position);
	vmin = vmax = vertices_[indices_[md.start_idx]].position;

	for (int i = 0; i < md.num_idx; ++i)
	{
		float extent = dot(principal_axis, vertices_[indices_[md.start_idx + i]].position);

		if ( extent > max)
		{
			max = extent;
			vmax = vertices_[indices_[md.start_idx + i]].position;
		}
		
		if ( extent < min)
		{
			min = extent;
			vmin = vertices_[indices_[md.start_idx + i]].position;
		}
	}

	/// Approximate bounding sphere
	vector3 center = 0.5f * (vmin + vmax);
	float   radius_sq = (center - vmin).sq_norm();
	float   radius = sqrt(radius_sq);

	/// Refine sphere
	for (int i = 0; i < md.num_idx; ++i)
	{
		vector3 point = vertices_[indices_[md.start_idx]].position;
		vector3 cp = center - point;
		if (cp.sq_norm() > radius_sq)
		{
			/// Adjust center and radius to encompass point
			cp.normalize();
			vector3 g = center - radius * cp;
			center = 0.5f * (g + point);
			radius_sq = (center - point).sq_norm();
			radius = sqrt(radius_sq);
		}
	}

#define BSPHERE_TEST
#ifdef BSPHERE_TEST
	float dev = radius_sq;
	for (int i = 0; i < md.num_idx; ++i)
	{
		vector3 point = vertices_[indices_[md.start_idx]].position;
		float dist_sq = (point - center).sq_norm();
		assert(dist_sq <= radius_sq);
		dev = std::min(dev, radius_sq - dist_sq);
	}
#endif

	md.bsphere.center = center;
	md.bsphere.radius = radius;
}

