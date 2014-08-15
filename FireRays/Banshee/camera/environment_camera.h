#ifndef ENVIRONMENT_CAMERA_H
#define ENVIRONMENT_CAMERA_H

#include "camera.h"
#include "../math/float3.h"


///< EnvironmentCamera is a camera mapping an image plane
///< to the sphere and looking into the whole sphere of directions.
///< Expect heavy distortion caused by spherical mapping.
///<
class EnvironmentCamera: public Camera
{
public:
    // TODO: implement angle limits
    // Pass camera position, camera aim, camera up vector, and depth limits
    EnvironmentCamera(float3 const& eye, float3 const& at, float3 const& up, float2 const& zcap);
    
    ///< sample is a value in [0,1] square describing where to sample the image plane
    void GenerateRay(float2 const& sample, ray& r) const;
    
private:
    // Camera frame
    float3 p_;
    float3 forward_;
    float3 right_;
    float3 up_;
    
    // Near and far Z
    float2 zcap_;
};

#endif // ENVIRONMENT_CAMERA_H
