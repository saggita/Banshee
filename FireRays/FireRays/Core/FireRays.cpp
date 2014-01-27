#include "FireRays.h"

std::unique_ptr<OCLRender> CreateRender()
{
	cl_platform_id platform;
	clGetPlatformIDs(1, &platform, nullptr);

	return std::unique_ptr<OCLRender>(new OCLRender(platform));
}