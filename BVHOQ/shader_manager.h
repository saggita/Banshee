//
//  shader_manager.h
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//
#ifndef BVHOQ_shader_manager_h
#define BVHOQ_shader_manager_h

#ifdef WIN32
#include <gl/glew.h>
#endif
#include <GLUT/GLUT.h>

#include <string>
#include <map>

class shader_manager
{
public:
    shader_manager();
    ~shader_manager();
    
    GLuint get_shader_program(std::string const& prog_name);
    
private:
    GLuint compile_program(std::string const& prog_name);
    
    shader_manager(shader_manager const&);
    shader_manager& operator = (shader_manager const&);
    
    std::map<std::string, GLuint> shader_cache_;
};


#endif

