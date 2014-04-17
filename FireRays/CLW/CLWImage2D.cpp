#include "CLWImage2D.h" 

#include "GLUT\GLUT.h"

CLWImage2D CLWImage2D::Create(cl_context context, cl_image_format const* imgFormat, size_t width, size_t height, size_t rowPitch)
{
    cl_int status = CL_SUCCESS;
    cl_mem deviceImg = clCreateImage2D(context, CL_MEM_READ_WRITE, imgFormat, width, height, rowPitch, nullptr, &status);
    
    assert(status == CL_SUCCESS);
    CLWImage2D image(deviceImg);
    
    clReleaseMemObject(deviceImg);
    
    return image;
}


CLWImage2D CLWImage2D::CreateFromGLTexture(cl_context context, cl_GLint texture)
{
    cl_int status = CL_SUCCESS;
    cl_mem deviceImg = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texture, &status);
    
    assert(status == CL_SUCCESS);
    CLWImage2D image(deviceImg);
    
    clReleaseMemObject(deviceImg);
    
    return image;
}

CLWImage2D::CLWImage2D(cl_mem image)
: ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>(image)
{
}

CLWImage2D::~CLWImage2D()
{
}