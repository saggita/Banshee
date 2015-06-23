#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLUT/GLUT.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#include <string>
#include <map>

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();
	
	GLuint GetProgram(std::string const& name);
	
private:
	GLuint CompileProgram(std::string const& name);
	
	ShaderManager(ShaderManager const&);
	ShaderManager& operator = (ShaderManager const&);
	
	std::map<std::string, GLuint> shadercache_;
};

#endif

