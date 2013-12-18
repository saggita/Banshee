//
//  ShaderManager.h
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

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();
	
	GLuint GetProgram(std::string const& progName);
	
private:
	GLuint CompileProgram(std::string const& progName);
	
	ShaderManager(ShaderManager const&);
	ShaderManager& operator = (ShaderManager const&);
	
	std::map<std::string, GLuint> shaderCache_;
};


#endif

