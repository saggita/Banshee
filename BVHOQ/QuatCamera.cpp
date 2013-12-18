//
//  QuatCamera.cpp
//  BVHOQ
//
//  Created by dmitryk on 17.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "QuatCamera.h"
#include "utils.h"

std::shared_ptr<QuatCamera> QuatCamera::LookAt(vector3 const& eye, vector3 const& at, vector3 const& up)
{
	std::shared_ptr<QuatCamera> camera = std::make_shared<QuatCamera>();
	camera->SetLookAt(eye, at, up);
	return camera;
}

QuatCamera::QuatCamera()
: tilt_()
{
}

QuatCamera::~QuatCamera()
{
}

void	QuatCamera::SetNearZ(float nz)
{
	zNear_ = nz;
}

void	QuatCamera::SetPixelSize(float pixelSize)
{
	pixelSize_ = pixelSize;
}

vector3 QuatCamera::GetPosition() const
{
	return p_;
}

float   QuatCamera::GetNearZ() const
{
	return zNear_;
}

float   QuatCamera::GetPixelSize() const
{
	return pixelSize_;
}

void	QuatCamera::SetFilmResolution(ui_size res)
{
	filmResolution_ = res;
}

ui_size QuatCamera::GetFilmResolution() const
{
	return filmResolution_;
}

vector3 QuatCamera::GetDirection() const
{
	matrix4x4 cameraMatrix = q_.to_matrix();
	return normalize(vector3(cameraMatrix(2,0), cameraMatrix(2,1), cameraMatrix(2,2)));
}

vector3 QuatCamera::GetUpVector() const
{
	matrix4x4 cameraMatrix = q_.to_matrix();
	return normalize(vector3(cameraMatrix(1,0), cameraMatrix(1,1), cameraMatrix(1,2)));
}

vector3 QuatCamera::GetRightVector() const
{
	matrix4x4 cameraMatrix = q_.to_matrix();
	return normalize(vector3(cameraMatrix(0,0), cameraMatrix(0,1), cameraMatrix(0,2)));
}

void QuatCamera::SetLookAt(vector3 const& eye, vector3 const& at, vector3 const& up)
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

void QuatCamera::Rotate(vector3 const& v, float angle)
{
	q_ *= rotation_quat(v, angle);
}

void QuatCamera::Rotate(float angle)
{
	Rotate(vector3(0.0, 1.0, 0.0), angle);
}

void QuatCamera::Tilt(float angle)
{
	if (abs(tilt_ + angle) < static_cast<real>(M_PI_2))
	{
		Rotate(GetRightVector(), angle);
	}
	
	tilt_ = std::max(std::min(tilt_ + angle, static_cast<real>(M_PI_2)), -static_cast<real>(M_PI_2));
}

void QuatCamera::MoveForward(float distance)
{
	p_ += distance * GetDirection();
}

matrix4x4 QuatCamera::GetViewMatrix() const
{
	matrix4x4 cameraMatrix = q_.to_matrix();
	vector3 u = vector3(cameraMatrix(1,0), cameraMatrix(1,1), cameraMatrix(1,2));
	vector3 v = vector3(cameraMatrix(2,0), cameraMatrix(2,1), cameraMatrix(2,2));

	return lookat_matrix_lh_dx(p_, p_ + v, u);
}

matrix4x4 QuatCamera::GetProjMatrix() const
{
	float width = filmResolution_.w * pixelSize_;
	float height = filmResolution_.h * pixelSize_;

	return perspective_proj_matrix_lh_gl(-width/2, width/2, -height/2, height/2, zNear_, 1000.f);
}

