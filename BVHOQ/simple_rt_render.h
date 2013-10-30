//
//  rt_cpu_gold.h
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVHOQ_simple_rt_render_h
#define BVHOQ_simple_rt_render_h

#include <vector>
#include <memory>
#include "render_base.h"
#include "bvh_accel.h"
#include "common_types.h"

#ifdef WIN32
#include <gl/glew.h>
#endif
#include <GLUT/GLUT.h>

class simple_rt_render : public render_base
{
public:
    simple_rt_render(){}
    ~simple_rt_render(){}
    
    void init(unsigned width, unsigned height);
    void commit(){}
    void render();
    
    GLuint output_texture() const;
    
private:
    unsigned width_;
    unsigned height_;
    
    std::shared_ptr<bvh_accel> accel_;
    
    std::vector<float> data_;
    unsigned gl_tex_;
};

#endif
