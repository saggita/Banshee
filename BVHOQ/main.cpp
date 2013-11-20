//
//  main.cpp
//  BVHOQ
//
//  Created by Dmitry Kozlov on 05.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//
#ifdef WIN32
#include <gl/glew.h>
#endif
#include <GLUT/GLUT.h>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>

#include "common_types.h"
#include "shader_manager.h"
#include "simple_rt_render.h"
#include "opencl_render.h"
#include "simple_camera.h"
#include "quaternion_camera.h"
#include "simple_scene.h"
#include "massive_scene.h"
#include "bvh_accel.h"
#include "mesh.h"


std::unique_ptr<shader_manager> g_shader_manager;
std::unique_ptr<render_base> g_render;
std::shared_ptr<quaternion_camera> g_camera;

static bool left_arrow_pressed = false;
static bool right_arrow_pressed = false;
static bool forward_arrow_pressed = false;
static bool back_arrow_pressed = false;
static vector2 mouse_pos = vector2(0,0);
static vector2 mouse_delta = vector2(0,0);

GLuint g_vertex_buffer_id;
GLuint g_index_buffer_id; 

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define CAMERA_POSITION vector3(0,0,0)
#define CAMERA_AT vector3(-1,0,0)
#define CAMERA_UP vector3(0,1,0)
#define CAMERA_NEAR_PLANE 0.1f
#define CAMERA_PIXEL_SIZE 0.00025f

void display()
{
	try
	{
		glClear(GL_COLOR_BUFFER_BIT);

		glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer_id);

		GLuint program = g_shader_manager->get_shader_program("simple");
		GLuint texture_loc = glGetUniformLocation(program, "g_Texture");
		assert(texture_loc >= 0);

		glUniform1i(texture_loc, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_render->output_texture());

		GLuint positionAttribId = glGetAttribLocation(program, "inPosition");
		GLuint texcoordAttribId = glGetAttribLocation(program, "inTexcoord");

		glVertexAttribPointer(positionAttribId, 3, GL_FLOAT, GL_FALSE, sizeof(float)*5, 0);
		glVertexAttribPointer(texcoordAttribId, 2, GL_FLOAT, GL_FALSE, sizeof(float)*5, (void*)(sizeof(float) * 3));

		/// Move away this state changes later
		glEnableVertexAttribArray(positionAttribId);
		glEnableVertexAttribArray(texcoordAttribId);

		glUseProgram(program);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		glutSwapBuffers();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what();
		exit(-1);
	}
}

void update()
{
    static auto prev_time = std::chrono::high_resolution_clock::now();
    auto curr_time = std::chrono::high_resolution_clock::now();
    auto time_delta = std::chrono::duration_cast<std::chrono::duration<double> >(curr_time - prev_time);

    
	float cam_rot_y = 0.f;
	float cam_rot_x = 0.f;

    const float MOUSE_SENSITIVITY = 0.0025f;
    vector2 delta = mouse_delta * vector2(MOUSE_SENSITIVITY, MOUSE_SENSITIVITY);
    cam_rot_x = -delta.y();
    cam_rot_y = -delta.x();
    
	if (cam_rot_y != 0.f)
		g_camera->rotate(cam_rot_y);
    
	if (cam_rot_x != 0.f)
		g_camera->tilt(cam_rot_x);
    
    const float MOVEMENT_SPEED = 0.01f;
	if (forward_arrow_pressed)
	{
		g_camera->move_forward((float)time_delta.count() * MOVEMENT_SPEED);
	}
    
	if (back_arrow_pressed)
	{
		g_camera->move_forward(-(float)time_delta.count() * MOVEMENT_SPEED);
	}
    
	g_render->commit();
	g_render->render();

	glutPostRedisplay();
}

void reshape(GLint w, GLint h)
{
}



void init_graphics()
{
	g_shader_manager.reset(new shader_manager());

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glCullFace(GL_NONE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glGenBuffers(1, &g_vertex_buffer_id);
	glGenBuffers(1, &g_index_buffer_id);

	// create vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer_id);

	float quad_vertex_data[] =
	{
		-1, -1, 0.5, 0, 0,
		1, -1, 0.5, 1, 0,
		1,  1, 0.5, 1, 1,
		-1,  1, 0.5, 0, 1
	};

	GLshort quad_index_data[] =
	{
		0, 1, 3,
		3, 1, 2
	};

	// fill data
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_index_data), quad_index_data, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void init_data()
{
	auto scene = massive_scene::create_from_obj("monkey.objm");
	g_camera = quaternion_camera::look_at(CAMERA_POSITION, CAMERA_AT, CAMERA_UP);
    
    g_camera->set_near_z(CAMERA_NEAR_PLANE);
    g_camera->set_pixel_size(CAMERA_PIXEL_SIZE);

	cl_platform_id platform;
	clGetPlatformIDs(1, &platform, nullptr);

	g_render.reset(new opencl_render(platform));
	//g_render.reset(new simple_rt_render());
	g_render->set_scene(scene);
	g_render->set_camera(g_camera);

	g_render->init(WINDOW_WIDTH, WINDOW_HEIGHT);
}




void on_mouse_move(int x, int y)
{
    mouse_delta = vector2(x,y) - mouse_pos;
    mouse_pos = vector2(x,y);
}

void on_key(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_UP:
            forward_arrow_pressed = true;
            break;
        case GLUT_KEY_DOWN:
            back_arrow_pressed = true;
            break;
        case GLUT_KEY_LEFT:
            left_arrow_pressed = true;
            break;
        case GLUT_KEY_RIGHT:
            right_arrow_pressed = true;
        default:
        break;
    }
}


void on_key_up(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_UP:
            forward_arrow_pressed = false;
            break;
        case GLUT_KEY_DOWN:
            back_arrow_pressed = false;
            break;
        case GLUT_KEY_LEFT:
            left_arrow_pressed = false;
            break;
        case GLUT_KEY_RIGHT:
            right_arrow_pressed = false;
        default:
            break;
    }
}

int main(int argc, const char * argv[])
{
	// GLUT Window Initialization:
	glutInit (&argc, (char**)argv);
	glutInitWindowSize (WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow ("test");

#ifdef WIN32
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::cout << "GLEW initialization failed\n";
		return -1;
	}
#endif

	try
	{
		init_graphics();
		init_data();

		// Register callbacks:
		glutDisplayFunc (display);
		glutReshapeFunc (reshape);

		glutSpecialFunc(on_key);
        glutSpecialUpFunc(on_key_up);
        glutPassiveMotionFunc(on_mouse_move);
		//glutMotionFunc(on_mouse_move);
		
		glutIdleFunc (update);
        
		// Create our popup menu
		// BuildPopupMenu ();
		// glutAttachMenu (GLUT_RIGHT_BUTTON);
		glutMainLoop ();

	}
	catch(std::runtime_error& err)
	{
		std::cout << err.what();
		return -1;
	}


	return 0;
}

