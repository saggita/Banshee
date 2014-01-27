//
//  CameraBase.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_CameraBase_h
#define BVHOQ_CameraBase_h

#include "CommonTypes.h"

class CameraBase
{
public:
	virtual ~CameraBase() = 0;
	
	virtual vector3 GetPosition() const = 0;
	virtual vector3 GetDirection() const = 0;
	virtual vector3 GetRightVector() const = 0;
	virtual vector3 GetUpVector() const = 0;
	virtual float   GetNearZ() const = 0;
	virtual float   GetPixelSize() const = 0;
	virtual ui_size GetFilmResolution() const = 0;

	virtual matrix4x4 GetViewMatrix() const = 0;
	virtual matrix4x4 GetProjMatrix() const = 0;
	
private:
	CameraBase& operator =(CameraBase const&);
};

inline CameraBase::~CameraBase(){};


#endif
