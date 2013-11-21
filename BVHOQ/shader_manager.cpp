//
//  shader_manager.cpp
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "shader_manager.h"
#include "utils.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>


static GLuint compile_shader(std::vector<GLchar> const& shader_source, GLenum type)
{
	GLuint shader = glCreateShader(type);
	
	GLint len				 = static_cast<GLint>(shader_source.size());
	GLchar const* source_array = &shader_source[0];
	
	glShaderSource(shader, 1, &source_array, &len);
	glCompileShader(shader);
	
	GLint result = GL_TRUE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	
	if(result == GL_FALSE)
	{
		std::vector<char> log;
		
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		
		log.resize(len);
		
		glGetShaderInfoLog(shader, len, &result, &log[0]);
		
		glDeleteShader(shader);
		
		throw std::runtime_error(std::string(log.begin(), log.end()));
		
		return 0;
	}
	
	return shader;
}


shader_manager::shader_manager()
{
	// testing code
	// core::uint program = GetShaderProgram("simple_ogl");
}


shader_manager::~shader_manager()
{
	for (auto citer = shader_cache_.cbegin(); citer != shader_cache_.cend(); ++citer)
	{
		glDeleteProgram(citer->second);
	}
}

GLuint shader_manager::compile_program(std::string const& prog_name)
{
	std::string vs_name = prog_name + ".vsh";
	std::string fs_name = prog_name + ".fsh";
	
	// Need to wrap the shader program here to be exception-safe
	std::vector<GLchar> source_code;
	
	load_file_contents(vs_name, source_code);
	GLuint vertex_shader = compile_shader(source_code, GL_VERTEX_SHADER);
	
	/// This part is not exception safe:
	/// if the VS compilation succeeded
	/// and PS compilation fails then VS object will leak
	/// fix this by wrapping the shaders into a class
	load_file_contents(fs_name, source_code);
	GLuint fragment_shader = compile_shader(source_code, GL_FRAGMENT_SHADER);
	
	GLuint program = glCreateProgram();
	
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	
	glLinkProgram(program);
	
	GLint result = GL_TRUE;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	
	if(result == GL_FALSE)
	{
		GLint length = 0;
		std::vector<char> log;
		
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		
		log.resize(length);
		
		glGetProgramInfoLog(program, length, &result, &log[0]);
		
		glDeleteProgram(program);
		
		throw std::runtime_error(std::string(log.begin(), log.end()));
	}
	
	return program;
}

GLuint shader_manager::get_shader_program(std::string const& prog_name)
{
	auto iter = shader_cache_.find(prog_name);
	
	if (iter != shader_cache_.end())
	{
		return iter->second;
	}
	else
	{
		GLuint program = compile_program(prog_name);
		shader_cache_[prog_name] = program;
		return program;
	}
}