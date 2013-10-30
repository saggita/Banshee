//
//  simple_camera.cpp
//  BVHOQ
//
//  Created by dmitryk on 08.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "simple_camera.h"

std::shared_ptr<simple_camera> simple_camera::look_at(vector3 const& pos, vector3 const& at, vector3 const& up, float nz, float pixel_size)
{
    vector3 v = normalize(at - pos);
	vector3 r = normalize(cross(normalize(up), v));
	vector3 u = normalize(cross(v, r));
    
    return std::make_shared<simple_camera>(pos, v, r, u, nz, pixel_size);
}

simple_camera::simple_camera(vector3 const& p, vector3 const& d, vector3 const& r, vector3 const& u, float nz, float pixel_size)
    : position_(p)
    , direction_(d)
    , right_(r)
    , up_(u)
    , near_z_(nz)
    , pixel_size_(pixel_size)
{
}

vector3 simple_camera::position() const
{
    return position_;
}

vector3 simple_camera::direction() const
{
    return  direction_;
}

vector3 simple_camera::right() const
{
    return right_;
}

vector3 simple_camera::up() const
{
    return up_;
}

float   simple_camera::near_z() const
{
    return near_z_;
}

float   simple_camera::pixel_size() const
{
    return pixel_size_;
}
