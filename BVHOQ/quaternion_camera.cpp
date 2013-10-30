//
//  quaternion_camera.cpp
//  BVHOQ
//
//  Created by dmitryk on 17.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "quaternion_camera.h"
#include "utils.h"

std::shared_ptr<quaternion_camera> quaternion_camera::look_at(vector3 const& eye, vector3 const& at, vector3 const& up)
{
    std::shared_ptr<quaternion_camera> camera = std::make_shared<quaternion_camera>();
    camera->lookat(eye, at, up);
    return camera;
}

quaternion_camera::quaternion_camera()
: tilt_()
{
}

quaternion_camera::~quaternion_camera()
{
}

void    quaternion_camera::set_near_z(float nz)
{
    near_z_ = nz;
}

void    quaternion_camera::set_pixel_size(float pixel_size)
{
    pixel_size_ = pixel_size;
}

vector3 quaternion_camera::position() const
{
    return p_;
}

float   quaternion_camera::near_z() const
{
    return near_z_;
}

float   quaternion_camera::pixel_size() const
{
    return pixel_size_;
}

vector3 quaternion_camera::direction() const
{
	matrix4x4 cameraMatrix = q_.to_matrix();
	return normalize(vector3(cameraMatrix(2,0), cameraMatrix(2,1), cameraMatrix(2,2)));
}

vector3 quaternion_camera::up() const
{
	matrix4x4 cameraMatrix = q_.to_matrix();
	return normalize(vector3(cameraMatrix(1,0), cameraMatrix(1,1), cameraMatrix(1,2)));
}

vector3 quaternion_camera::right() const
{
	matrix4x4 cameraMatrix = q_.to_matrix();
	return normalize(vector3(cameraMatrix(0,0), cameraMatrix(0,1), cameraMatrix(0,2)));
}

void quaternion_camera::lookat(vector3 const& eye, vector3 const& at, vector3 const& up)
{
	vector3 v = normalize(at - eye);
	vector3 r = normalize(cross(normalize(up), v));
	vector3 u = normalize(cross(v, r));
    
	/// matrix should have basis vectors in rows
	/// to be used for quaternion construction
	/// would be good to add several options
	/// to quaternion class
	matrix4x4 cameraMatrix = matrix4x4(r.x(), u.x() , v.x() , 0,
                                       r.y(), u.y(), v.y(), 0,
                                       r.z(), u.z(), v.z(), 0,
                                       0, 0, 0, 1);
    
	q_ = normalize(quat(cameraMatrix));
    
	p_ = eye;
}

void quaternion_camera::rotate_camera(vector3 const& v, float angle)
{
	q_ *= rotation_quat(v, angle);
}

void quaternion_camera::rotate(float angle)
{
	rotate_camera(vector3(0.0, 1.0, 0.0), angle);
}

void quaternion_camera::tilt(float angle)
{
	if (abs(tilt_ + angle) < static_cast<real>(M_PI_2))
	{
		rotate_camera(right(), angle);
	}
    
	tilt_ = std::max(std::min(tilt_ + angle, static_cast<real>(M_PI_2)), -static_cast<real>(M_PI_2));
}

void quaternion_camera::move_forward(float distance)
{
	p_ += distance * direction();
}

