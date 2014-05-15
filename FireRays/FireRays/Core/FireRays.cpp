#include "FireRays.h"

#include "OCLRender.h"

std::unique_ptr<RenderBase> CreateRender()
{
	cl_platform_id platform[1];
	clGetPlatformIDs(1, platform, nullptr);

	return std::unique_ptr<RenderBase>(new OCLRender(platform[0]));
}