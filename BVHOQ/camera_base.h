//
//  camera_base.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_camera_base_h
#define BVHOQ_camera_base_h

#include "common_types.h"

class camera_base
{
public:
    virtual ~camera_base() = 0;
    
    virtual vector3 position() const = 0;
    virtual vector3 direction() const = 0;
    virtual vector3 right() const = 0;
    virtual vector3 up() const = 0;
    virtual float   near_z() const = 0;
    virtual float   pixel_size() const = 0;
    
private:
    camera_base& operator =(camera_base const&);
};

inline camera_base::~camera_base(){};


#endif
