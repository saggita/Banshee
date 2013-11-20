//
//  rt_cpu_gold.cpp
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "simple_rt_render.h"

#include "thread_pool.h"

#include <limits>
#include <chrono>
#include <iostream>

#include "render_base.h"
#include "camera_base.h"

#include "bbox.h"

#define MULTITHREADED

GLuint simple_rt_render::output_texture() const
{
    return gl_tex_;
}

void simple_rt_render::init(unsigned width, unsigned height)
{
    width_ = width;
    height_ = height;
    
    data_.resize(width_ * height_ * 4);
    
    accel_ = bvh_accel::create_from_scene(*scene());
    
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &gl_tex_);
    
    glBindTexture(GL_TEXTURE_2D, gl_tex_);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width_, height_,
                 0, GL_RGBA, GL_FLOAT, nullptr);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void simple_rt_render::render()
{
#ifdef MULTITHREADED
	static thread_pool<float> pool;
	static std::vector<std::future<float> > futures(width_*height_);
#endif
    auto start_time = std::chrono::high_resolution_clock::now();
    
    vector3 v = camera()->direction();
	vector3 r = camera()->right();
	vector3 u = camera()->up();
    
    float pixel_size = camera()->pixel_size();
    
    for (unsigned i = 0; i < width_; ++i)
        for (unsigned j = 0; j < height_; ++j)
        {
#ifndef MULTITHREADED
            int ii = i - (width_ >> 1);
            int jj = j - (height_ >> 1);
            
            vector3 dir = normalize(v * camera()->near_z() + u * (jj + 0.5f) * pixel_size + r * (ii + 0.5f) * pixel_size);
            vector3 orig = camera()->position();
            
            ray rr(orig, dir);
            
            float t = 50.f;
            data_[j * (width_ * 4) + i * 4] = t;
            data_[j * (width_ * 4) + i * 4 + 1] = t;
            data_[j * (width_ * 4) + i * 4 + 2] = t;
            data_[j * (width_ * 4) + i * 4 + 3] = t;
            
            if (accel_->intersect(rr, t))
            {
                t /= 30.f;
                data_[j * (width_ * 4) + i * 4] = t;
                data_[j * (width_ * 4) + i * 4 + 1] = t;
                data_[j * (width_ * 4) + i * 4 + 2] = t;
                data_[j * (width_ * 4) + i * 4 + 3] = t;
            }
#else
			float t = 50.f;
            data_[j * (width_ * 4) + i * 4] = t;
            data_[j * (width_ * 4) + i * 4 + 1] = t;
            data_[j * (width_ * 4) + i * 4 + 2] = t;
            data_[j * (width_ * 4) + i * 4 + 3] = t;

			futures[j*width_ + i] = std::move(pool.submit([=]()->float
			{
				int ii = i - (width_ >> 1);
				int jj = j - (height_ >> 1);
            
				float t = 50.f;
				vector3 dir = normalize(v * camera()->near_z() + u * (jj + 0.5f) * pixel_size + r * (ii + 0.5f) * pixel_size);
				vector3 orig = camera()->position();
            
				ray rr(orig, dir);
            
				if (accel_->intersect(rr, t))
				{
					t /= 30.f;	
					data_[j * (width_ * 4) + i * 4] = t;
					data_[j * (width_ * 4) + i * 4 + 1] = t;
					data_[j * (width_ * 4) + i * 4 + 2] = t;
					data_[j * (width_ * 4) + i * 4 + 3] = t;
				}
				return t;
			}));
#endif
		}


#ifdef MULTITHREADED
		pool.wait();
#endif

    glBindTexture(GL_TEXTURE_2D, gl_tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width_, height_,
                 0, GL_RGBA, GL_FLOAT, &data_[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    auto delta_time = std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now() - start_time).count();
    std::cout << "cpu time " << delta_time << " seconds\n";
}

