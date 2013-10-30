//
//  simple_camera.h
//  BVHOQ
//
//  Created by dmitryk on 08.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__simple_camera__
#define __BVHOQ__simple_camera__

#include "camera_base.h"
#include <memory>

class simple_camera : public camera_base
{
public:
    static std::shared_ptr<simple_camera> look_at(vector3 const& pos, vector3 const& at, vector3 const& up, float nz, float pixel_size);
    simple_camera(vector3 const& p, vector3 const& d, vector3 const& r, vector3 const& u, float nz, float pixel_size);
    
    vector3 position() const;
    vector3 direction() const;
    vector3 right() const;
    
    vector3 up() const;
    float   near_z() const;
    float   pixel_size() const;
    
private:
    vector3 position_;
    vector3 direction_;
    vector3 right_;
    vector3 up_;
    
    float near_z_;
    float pixel_size_;
};

#endif /* defined(__BVHOQ__simple_camera__) */
