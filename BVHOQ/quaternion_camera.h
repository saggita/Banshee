//
//  quaternion_camera.h
//  BVHOQ
//
//  Created by dmitryk on 17.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__quaternion_camera__
#define __BVHOQ__quaternion_camera__

#include <memory>

#include "camera_base.h"
#include "common_types.h"


class quaternion_camera : public camera_base
{
public:
	static std::shared_ptr<quaternion_camera> look_at(vector3 const& eye, vector3 const& at, vector3 const& up);
	
	quaternion_camera();
	~quaternion_camera();
	
	void rotate(float angle);
	void tilt(float angle);
	void move_forward(float distance);
	
	vector3 position() const;
	vector3 direction() const;
	vector3 right() const;
	vector3 up() const;
	
	void	set_near_z(float nz);
	void	set_pixel_size(float pixel_size);
	void	set_film_resolution(ui_size res);

	float   near_z() const;
	float   pixel_size() const;
	ui_size film_resolution() const;

	matrix4x4 view_matrix() const;
	matrix4x4 proj_matrix() const;

protected:
	void rotate_camera(vector3 const& v, float angle);
	void lookat(vector3 const& eye, vector3 const& at, vector3 const& up);
	
private:
	quat q_;
	vector3 p_;
	
	// Angle to impose tilt constraint
	float tilt_;
	float near_z_;
	float pixel_size_;
	ui_size film_resolution_;
};


#endif /* defined(__BVHOQ__quaternion_camera__) */
