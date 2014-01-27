//
//  QuatCamera.h
//  BVHOQ
//
//  Created by dmitryk on 17.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__quaternion_camera__
#define __BVHOQ__quaternion_camera__

#include <memory>

#include "FireRays.h"


class QuatCamera : public CameraBase
{
public:
	static std::shared_ptr<QuatCamera> LookAt(vector3 const& eye, vector3 const& at, vector3 const& up);
	
	QuatCamera();
	~QuatCamera();
	
	void Rotate(float angle);
	void Tilt(float angle);
	void MoveForward(float distance);
	
	vector3 GetPosition() const;
	vector3 GetDirection() const;
	vector3 GetRightVector() const;
	vector3 GetUpVector() const;
	
	void	SetNearZ(float nz);
	void	SetPixelSize(float pixelSize);
	void	SetFilmResolution(ui_size res);

	float   GetNearZ() const;
	float   GetPixelSize() const;
	ui_size GetFilmResolution() const;

	matrix4x4 GetViewMatrix() const;
	matrix4x4 GetProjMatrix() const;

protected:
	void Rotate(vector3 const& v, float angle);
	void SetLookAt(vector3 const& eye, vector3 const& at, vector3 const& up);
	
private:
	quat q_;
	vector3 p_;
	
	// Angle to impose Tilt constraint
	float tilt_;
	float zNear_;
	float pixelSize_;
	ui_size filmResolution_;
};


#endif /* defined(__BVHOQ__quaternion_camera__) */
