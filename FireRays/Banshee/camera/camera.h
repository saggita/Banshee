#ifndef CAMERA_H
#define CAMERA_H

#include "../math/float2.h"
#include "../math/ray.h"

///< Camera interface is provided for the renderer to generate rays.
///< The camera accepts sample parameter which is [0,1]x[0,1] value
///< describing where the image plane has to be sampled. 
///< Note that the camera doesn't have any information about
///< image plane, its resolution, etc. It is renderers responsibility
///< to remap this values into the image plane.
///<
class Camera
{
public:
    /// Destructor
    virtual ~Camera(){}

    ///< sample is a value in [0,1] square describing where to sample the image plane
    virtual void GenerateRay(float2 const& sample, ray& r) const = 0;
};

#endif // CAMERA_H