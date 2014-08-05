#ifndef IMAGEPLANE_H
#define IMAGEPLANE_H

#include "../math/float3.h"
#include "../math/int2.h"

///< ImagePlane class represents an image plane and
///< is designed for the Renderer to write its result.
///< Note that default image plane doesn't guarantee
///< atomicity of operations.
///<
class ImagePlane
{
public:
    virtual ~ImagePlane(){}

    // This method is called by the renderer prior to adding samples
    virtual void Prepare(){};

    // This method is called by the renderer after adding all the samples
    virtual void Finalize(){}

    // Add weighted color contribution to the image plane
    virtual void AddSample(float2 const& sample, float w, float3 value) = 0;

    // This is used by the renderer to decide on the number of samples needed
    virtual int2 resolution() const = 0;
};


#endif