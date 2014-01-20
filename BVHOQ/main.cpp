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
#include <sstream>

#include "CommonTypes.h"
#include "ShaderManager.h"
#include "OCLRender.h"
#include "QuatCamera.h"
#include "SimpleScene.h"
#include "MassiveScene.h"
#include "BVHAccelerator.h"
#include "Mesh.h"
#include "utils.h"


std::unique_ptr<ShaderManager>	gShaderManager;
std::unique_ptr<OCLRender>		gRender;
std::shared_ptr<SceneBase>		gScene;
std::shared_ptr<QuatCamera>		gCamera;

static bool gIsLeftPressed = false;
static bool gIsRightPressed = false;
static bool gIsFwdPressed = false;
static bool gIsBackPressed = false;
static vector2 gMousePosition = vector2(0,0);
static vector2 gMouseDelta = vector2(0,0);

GLuint gVertexBufferId;
GLuint gIndexBufferId;

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define CAMERA_POSITION vector3(1,1,2)
#define CAMERA_AT vector3(0,0,0)
#define CAMERA_UP vector3(0,1,0)
#define CAMERA_NEAR_PLANE 0.01f
#define CAMERA_PIXEL_SIZE 0.000025f


struct PointLightData
{
	vector4 pos;
	vector4 color;
};

struct SpotLightData
{
	vector4 pos;
	vector4 dir;
	vector4 color;
	vector4 angle;
};

std::vector<PointLightData> gPointLights;
std::vector<SpotLightData>  gSpotLights;

void Display()
{
	try
	{
		{
            glDisable(GL_DEPTH_TEST);
			glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

			glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferId);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBufferId);

			GLuint program = gShaderManager->GetProgram("simple");
			glUseProgram(program);

			GLuint textureLocation = glGetUniformLocation(program, "g_Texture");
			assert(textureLocation >= 0);

			glUniform1i(textureLocation, 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gRender->GetOutputTexture());

			GLuint positionAttribId = glGetAttribLocation(program, "inPosition");
			GLuint texcoordAttribId = glGetAttribLocation(program, "inTexcoord");

			glVertexAttribPointer(positionAttribId, 3, GL_FLOAT, GL_FALSE, sizeof(float)*5, 0);
			glVertexAttribPointer(texcoordAttribId, 2, GL_FLOAT, GL_FALSE, sizeof(float)*5, (void*)(sizeof(float) * 3));

			glEnableVertexAttribArray(positionAttribId);
			glEnableVertexAttribArray(texcoordAttribId);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

			glDisableVertexAttribArray(texcoordAttribId);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glUseProgram(0);
		}

		glutSwapBuffers();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what();
		exit(-1);
	}
}

void Update()
{
	static auto prevTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double> >(currentTime - prevTime);


	float cameraRotationY = 0.f;
	float cameraRotationX = 0.f;

	const float kMouseSensitivity = 0.00125f;
	vector2 delta = gMouseDelta * vector2(kMouseSensitivity, kMouseSensitivity);
	cameraRotationX = -delta.y();
	cameraRotationY = -delta.x();

	if (cameraRotationY != 0.f)
    {
		gCamera->Rotate(cameraRotationY);
        gRender->FlushFrame();
    }

	if (cameraRotationX != 0.f)
    {
		gCamera->Tilt(cameraRotationX);
        gRender->FlushFrame();
    }

	const float kMovementSpeed = 0.01f;
	if (gIsFwdPressed)
	{
		gCamera->MoveForward((float)deltaTime.count() * kMovementSpeed);
        gRender->FlushFrame();
	}

	if (gIsBackPressed)
	{
		gCamera->MoveForward(-(float)deltaTime.count() * kMovementSpeed);
        gRender->FlushFrame();
	}

	gRender->Commit();
	gRender->Render();

	glutPostRedisplay();
}

void Reshape(GLint w, GLint h)
{
}


void InitGraphics()
{
	gShaderManager.reset(new ShaderManager());

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glCullFace(GL_NONE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glGenBuffers(1, &gVertexBufferId);
	glGenBuffers(1, &gIndexBufferId);

	// create Vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBufferId);

	float quadVertexData[] =
	{
		-1, -1, 0.5, 0, 0,
		1, -1, 0.5, 1, 0,
		1,  1, 0.5, 1, 1,
		-1,  1, 0.5, 0, 1
	};

	GLshort quadIndexData[] =
	{
		0, 1, 3,
		3, 1, 2
	};

	// fill data
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertexData), quadVertexData, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndexData), quadIndexData, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void InitData()
{
    rand_init();
    
	//gScene = SimpleScene::CreateFromObj("sibenik.objm");
	gScene = MassiveScene::Create();
	gCamera = QuatCamera::LookAt(CAMERA_POSITION, CAMERA_AT, CAMERA_UP);

	gCamera->SetNearZ(CAMERA_NEAR_PLANE);
	gCamera->SetPixelSize(CAMERA_PIXEL_SIZE);
	gCamera->SetFilmResolution(ui_size(WINDOW_WIDTH, WINDOW_HEIGHT));

	cl_platform_id platform;
	clGetPlatformIDs(1, &platform, nullptr);

	gRender.reset(new OCLRender(platform));
	//gRender.reset(new simple_rt_render());
	gRender->SetScene(gScene);
	gRender->SetCamera(gCamera);

	gRender->Init(WINDOW_WIDTH, WINDOW_HEIGHT);
}


void OnMouseMove(int x, int y)
{
	gMouseDelta = vector2(x,y) - gMousePosition;
	gMousePosition = vector2(x,y);
}

void OnKey(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		gIsFwdPressed = true;
		break;
	case GLUT_KEY_DOWN:
		gIsBackPressed = true;
		break;
	case GLUT_KEY_LEFT:
		gIsLeftPressed = true;
		break;
	case GLUT_KEY_RIGHT:
		gIsRightPressed = true;
	default:
		break;
	}
}

void OnKeyUp(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		gIsFwdPressed = false;
		break;
	case GLUT_KEY_DOWN:
		gIsBackPressed = false;
		break;
	case GLUT_KEY_LEFT:
		gIsLeftPressed = false;
		break;
	case GLUT_KEY_RIGHT:
		gIsRightPressed = false;
	default:
		break;
	}
}

int main(int argc, const char * argv[])
{
	// GLUT Window Initialization:
	glutInit (&argc, (char**)argv);
	glutInitWindowSize (WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
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
		InitGraphics();
		InitData();

		// Register callbacks:
		glutDisplayFunc (Display);
		glutReshapeFunc (Reshape);
		glutSpecialFunc(OnKey);
		glutSpecialUpFunc(OnKeyUp);
		glutPassiveMotionFunc(OnMouseMove);
		glutIdleFunc (Update);
		glutMainLoop ();

	}
	catch(std::runtime_error& err)
	{
		std::cout << err.what();
		return -1;
	}


	return 0;
}

